// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCombatManagerSubsystem.h"
#include "IPBCombatParticipant.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/ProjectB3.h"

void UPBCombatManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CombatState = EPBCombatState::OutOfCombat;
	CurrentTurnIndex = 0;
	CurrentRound = 0;
	NextSharedGroupId = 0;
}

void UPBCombatManagerSubsystem::Deinitialize()
{
	if (IsInCombat())
	{
		EndCombat();
	}

	Super::Deinitialize();
}

// ──────────────────────── 전투 시작/종료 ────────────────────────

void UPBCombatManagerSubsystem::StartCombat(const TArray<AActor*>& Combatants)
{
	if (IsInCombat())
	{
		UE_LOG(LogProjectB3, Warning, TEXT("CombatManager: 이미 전투 중입니다."));
		return;
	}

	if (Combatants.Num() < 2)
	{
		UE_LOG(LogProjectB3, Warning, TEXT("CombatManager: 전투 참가자가 2명 미만입니다."));
		return;
	}

	// 참가자 등록
	RegisteredCombatants.Empty();
	for (AActor* Combatant : Combatants)
	{
		if (IsValid(Combatant) && Cast<IPBCombatParticipant>(Combatant))
		{
			RegisteredCombatants.Add(Combatant);
		}
	}

	// CombatStarting: 모든 참가자에게 OnCombatBegin 통지
	SetCombatState(EPBCombatState::CombatStarting);

	for (const TWeakObjectPtr<AActor>& WeakCombatant : RegisteredCombatants)
	{
		if (AActor* Combatant = WeakCombatant.Get())
		{
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Combatant))
			{
				Participant->OnCombatBegin();
			}
		}
	}

	// InitiativeRoll: 이니셔티브 굴림 → 정렬 → 공유 턴 그룹 계산
	SetCombatState(EPBCombatState::InitiativeRoll);

	RollInitiative();
	CalculateSharedTurnGroups();

	// 라운드 1 시작
	CurrentRound = 1;
	CurrentTurnIndex = 0;
	OnRoundChanged.Broadcast(CurrentRound);

	// 모든 참가자에게 OnRoundBegin (Reaction 리필)
	for (const TWeakObjectPtr<AActor>& WeakCombatant : RegisteredCombatants)
	{
		if (AActor* Combatant = WeakCombatant.Get())
		{
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Combatant))
			{
				Participant->OnRoundBegin();
			}
		}
	}

	// 첫 번째 턴 시작
	SetCombatState(EPBCombatState::TurnInProgress);
	BeginTurnForCurrentEntry();
}

void UPBCombatManagerSubsystem::EndCombat()
{
	SetCombatState(EPBCombatState::CombatEnding);

	// 모든 참가자에게 OnCombatEnd 통지
	for (const TWeakObjectPtr<AActor>& WeakCombatant : RegisteredCombatants)
	{
		if (AActor* Combatant = WeakCombatant.Get())
		{
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Combatant))
			{
				Participant->OnCombatEnd();
			}
		}
	}

	// 상태 정리
	RegisteredCombatants.Empty();
	InitiativeOrder.Empty();
	CurrentTurnIndex = 0;
	CurrentRound = 0;
	NextSharedGroupId = 0;
	SharedTurnActiveIndex = 0;
	GroupActedMemberIndices.Empty();

	SetCombatState(EPBCombatState::OutOfCombat);
}

void UPBCombatManagerSubsystem::NotifyCombatantIncapacitated(AActor* Combatant)
{
	if (!IsValid(Combatant) || !IsInCombat())
	{
		return;
	}

	OnCombatantIncapacitated.Broadcast(Combatant);
	HandleIncapacitated(Combatant);
}

// ──────────────────────── 턴 흐름 제어 ────────────────────────

void UPBCombatManagerSubsystem::EndCurrentTurn()
{
	if (CombatState != EPBCombatState::TurnInProgress)
	{
		return;
	}

	if (!InitiativeOrder.IsValidIndex(CurrentTurnIndex))
	{
		return;
	}

	const FPBInitiativeEntry& CurrentEntry = InitiativeOrder[CurrentTurnIndex];
	int32 CurrentGroupId = CurrentEntry.SharedTurnGroupId;

	if (CurrentGroupId >= 0)
	{
		// 공유 턴 그룹: 현재 멤버를 행동 완료로 기록하고 다음 멤버 탐색
		GroupActedMemberIndices.Add(SharedTurnActiveIndex);

		TArray<AActor*> Group = GetCurrentSharedTurnGroup();
		int32 NextActiveIndex = INDEX_NONE;

		// 현재 인덱스 이후부터 순회하여 다음 활성 멤버 탐색
		for (int32 i = SharedTurnActiveIndex + 1; i < Group.Num(); ++i)
		{
			if (GroupActedMemberIndices.Contains(i))
			{
				continue;
			}
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Group[i]))
			{
				if (!Participant->IsIncapacitated())
				{
					NextActiveIndex = i;
					break;
				}
			}
		}

		// 현재 인덱스 이전에서도 미행동 멤버 탐색 (래핑)
		if (NextActiveIndex == INDEX_NONE)
		{
			for (int32 i = 0; i < SharedTurnActiveIndex; ++i)
			{
				if (GroupActedMemberIndices.Contains(i))
				{
					continue;
				}
				if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Group[i]))
				{
					if (!Participant->IsIncapacitated())
					{
						NextActiveIndex = i;
						break;
					}
				}
			}
		}

		if (NextActiveIndex != INDEX_NONE)
		{
			// 그룹 내 다음 멤버로 전환 (그룹 턴 유지)
			SharedTurnActiveIndex = NextActiveIndex;
			if (IPBCombatParticipant* NextParticipant = Cast<IPBCombatParticipant>(Group[NextActiveIndex]))
			{
				NextParticipant->OnTurnActivated();
			}
			OnActiveTurnChanged.Broadcast(Group[NextActiveIndex], CurrentTurnIndex);
			return;
		}

		// 그룹 내 모든 멤버가 행동 완료 → 그룹 전원 OnTurnEnd
		SetCombatState(EPBCombatState::TurnEnding);
		for (const FPBInitiativeEntry& Entry : InitiativeOrder)
		{
			if (Entry.SharedTurnGroupId == CurrentGroupId)
			{
				if (AActor* Actor = Entry.Combatant.Get())
				{
					if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Actor))
					{
						Participant->OnTurnEnd();
					}
				}
			}
		}
	}
	else
	{
		// 개별 턴: 현재 멤버에게 OnTurnEnd
		SetCombatState(EPBCombatState::TurnEnding);
		if (AActor* Actor = CurrentEntry.Combatant.Get())
		{
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Actor))
			{
				Participant->OnTurnEnd();
			}
		}
	}

	// 다음 턴으로 진행
	AdvanceToNextTurn();
}

AActor* UPBCombatManagerSubsystem::GetCurrentCombatant() const
{
	if (!InitiativeOrder.IsValidIndex(CurrentTurnIndex))
	{
		return nullptr;
	}

	const FPBInitiativeEntry& CurrentEntry = InitiativeOrder[CurrentTurnIndex];
	int32 GroupId = CurrentEntry.SharedTurnGroupId;

	if (GroupId >= 0)
	{
		// 공유 턴 그룹에서 현재 활성 멤버 반환
		TArray<AActor*> Group = GetCurrentSharedTurnGroup();
		if (Group.IsValidIndex(SharedTurnActiveIndex))
		{
			return Group[SharedTurnActiveIndex];
		}
	}

	return CurrentEntry.Combatant.Get();
}

TArray<AActor*> UPBCombatManagerSubsystem::GetCurrentSharedTurnGroup() const
{
	TArray<AActor*> Group;

	if (!InitiativeOrder.IsValidIndex(CurrentTurnIndex))
	{
		return Group;
	}

	int32 GroupId = InitiativeOrder[CurrentTurnIndex].SharedTurnGroupId;
	if (GroupId < 0)
	{
		// 개별 턴인 경우 현재 참가자만 반환
		if (AActor* Actor = InitiativeOrder[CurrentTurnIndex].Combatant.Get())
		{
			Group.Add(Actor);
		}
		return Group;
	}

	for (const FPBInitiativeEntry& Entry : InitiativeOrder)
	{
		if (Entry.SharedTurnGroupId == GroupId)
		{
			if (AActor* Actor = Entry.Combatant.Get())
			{
				Group.Add(Actor);
			}
		}
	}

	return Group;
}

void UPBCombatManagerSubsystem::SwitchToGroupMember(AActor* TargetMember)
{
	if (!IsValid(TargetMember) || CombatState != EPBCombatState::TurnInProgress)
	{
		return;
	}

	TArray<AActor*> Group = GetCurrentSharedTurnGroup();
	int32 FoundIndex = Group.IndexOfByKey(TargetMember);
	if (FoundIndex != INDEX_NONE)
	{
		SharedTurnActiveIndex = FoundIndex;
		// 새로 활성화된 멤버에게 행동 차례 통지
		if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(TargetMember))
		{
			Participant->OnTurnActivated();
		}
		OnActiveTurnChanged.Broadcast(TargetMember, CurrentTurnIndex);
	}
}

// ──────────────────────── 반응 행동 ────────────────────────

void UPBCombatManagerSubsystem::TriggerReaction(const FPBReactionContext& Context)
{
	if (CombatState != EPBCombatState::TurnInProgress)
	{
		return;
	}

	AActor* ReactingActor = Context.ReactingActor.Get();
	if (!IsValid(ReactingActor))
	{
		return;
	}

	IPBCombatParticipant* Reactor = Cast<IPBCombatParticipant>(ReactingActor);
	if (!Reactor || !Reactor->CanReact())
	{
		return;
	}

	PendingReactionContext = Context;
	SetCombatState(EPBCombatState::WaitingForReaction);
	OnReactionTriggered.Broadcast(Context);

	Reactor->OnReactionOpportunity(Context);
}

void UPBCombatManagerSubsystem::ResolveReaction(bool bUsedReaction)
{
	if (CombatState != EPBCombatState::WaitingForReaction)
	{
		return;
	}

	if (bUsedReaction)
	{
		// 현재 턴 소유자에게 OnActionInterrupted 호출
		if (AActor* CurrentActor = GetCurrentCombatant())
		{
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(CurrentActor))
			{
				Participant->OnActionInterrupted();
			}
		}
	}

	PendingReactionContext = FPBReactionContext();
	SetCombatState(EPBCombatState::TurnInProgress);
}

// ──────────────────────── 상태 조회 ────────────────────────

bool UPBCombatManagerSubsystem::IsInCombat() const
{
	return CombatState != EPBCombatState::OutOfCombat;
}

// ──────────────────────── 이니셔티브 ────────────────────────

void UPBCombatManagerSubsystem::RollInitiative()
{
	InitiativeOrder.Empty();

	for (const TWeakObjectPtr<AActor>& WeakCombatant : RegisteredCombatants)
	{
		AActor* Combatant = WeakCombatant.Get();
		IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Combatant);
		if (!IsValid(Combatant) || !Participant)
		{
			continue;
		}

		FPBInitiativeEntry Entry;
		Entry.Combatant = Combatant;
		Entry.DexModifier = Participant->GetInitiativeModifier();

		bool bAdvantage = Participant->HasInitiativeAdvantage();
		Entry.RawRoll = RollD20(bAdvantage, /*bDisadvantage=*/ false);
		Entry.InitiativeTotal = Entry.RawRoll + Entry.DexModifier;

		InitiativeOrder.Add(Entry);
	}

	SortInitiativeOrder();
}

int32 UPBCombatManagerSubsystem::RollD20(bool bAdvantage, bool bDisadvantage) const
{
	int32 Roll1 = FMath::RandRange(1, 20);

	// 이점과 불이점이 상쇄되면 일반 굴림
	if (bAdvantage == bDisadvantage)
	{
		return Roll1;
	}

	int32 Roll2 = FMath::RandRange(1, 20);

	if (bAdvantage)
	{
		return FMath::Max(Roll1, Roll2);
	}

	return FMath::Min(Roll1, Roll2);
}

void UPBCombatManagerSubsystem::SortInitiativeOrder()
{
	InitiativeOrder.StableSort([](const FPBInitiativeEntry& A, const FPBInitiativeEntry& B)
	{
		if (A.InitiativeTotal != B.InitiativeTotal)
		{
			return A.InitiativeTotal > B.InitiativeTotal;
		}
		if (A.DexModifier != B.DexModifier)
		{
			return A.DexModifier > B.DexModifier;
		}
		return A.RawRoll > B.RawRoll;
	});
}

// ──────────────────────── 공유 턴 그룹 ────────────────────────

void UPBCombatManagerSubsystem::CalculateSharedTurnGroups()
{
	NextSharedGroupId = 0;

	if (InitiativeOrder.Num() == 0)
	{
		return;
	}

	// 첫 번째 엔트리는 항상 새 그룹 시작
	int32 CurrentGroupId = NextSharedGroupId++;
	int32 GroupMemberCount = 1;
	InitiativeOrder[0].SharedTurnGroupId = CurrentGroupId;

	FGameplayTag PrevFaction;
	if (IPBCombatParticipant* Prev = Cast<IPBCombatParticipant>(InitiativeOrder[0].Combatant.Get()))
	{
		PrevFaction = Prev->GetFactionTag();
	}

	for (int32 i = 1; i < InitiativeOrder.Num(); ++i)
	{
		FGameplayTag CurrentFaction;
		if (IPBCombatParticipant* Current = Cast<IPBCombatParticipant>(InitiativeOrder[i].Combatant.Get()))
		{
			CurrentFaction = Current->GetFactionTag();
		}

		if (CurrentFaction.IsValid() && CurrentFaction == PrevFaction)
		{
			// 같은 진영 — 같은 그룹
			InitiativeOrder[i].SharedTurnGroupId = CurrentGroupId;
			GroupMemberCount++;
		}
		else
		{
			// 이전 그룹이 1명뿐이면 개별 턴으로 설정
			if (GroupMemberCount == 1)
			{
				InitiativeOrder[i - 1].SharedTurnGroupId = -1;
			}

			// 새 그룹 시작
			CurrentGroupId = NextSharedGroupId++;
			GroupMemberCount = 1;
			InitiativeOrder[i].SharedTurnGroupId = CurrentGroupId;
			PrevFaction = CurrentFaction;
		}
	}

	// 마지막 그룹이 1명뿐이면 개별 턴
	if (GroupMemberCount == 1)
	{
		InitiativeOrder.Last().SharedTurnGroupId = -1;
	}
}

// ──────────────────────── 턴 진행 ────────────────────────

void UPBCombatManagerSubsystem::SetCombatState(EPBCombatState NewState)
{
	if (CombatState == NewState)
	{
		return;
	}

	CombatState = NewState;
	OnCombatStateChanged.Broadcast(NewState);
}

void UPBCombatManagerSubsystem::AdvanceToNextTurn()
{
	if (InitiativeOrder.Num() == 0)
	{
		return;
	}

	// 현재 공유 턴 그룹이면 그룹 끝까지 점프
	const FPBInitiativeEntry& CurrentEntry = InitiativeOrder[CurrentTurnIndex];
	int32 CurrentGroupId = CurrentEntry.SharedTurnGroupId;

	if (CurrentGroupId >= 0)
	{
		// 그룹의 마지막 멤버 인덱스를 찾아 그 다음으로 이동
		int32 LastGroupIndex = CurrentTurnIndex;
		for (int32 i = CurrentTurnIndex + 1; i < InitiativeOrder.Num(); ++i)
		{
			if (InitiativeOrder[i].SharedTurnGroupId == CurrentGroupId)
			{
				LastGroupIndex = i;
			}
			else
			{
				break;
			}
		}
		CurrentTurnIndex = LastGroupIndex + 1;
	}
	else
	{
		CurrentTurnIndex++;
	}

	SharedTurnActiveIndex = 0;

	// 라운드 끝 확인
	if (CurrentTurnIndex >= InitiativeOrder.Num())
	{
		HandleRoundEnd();
		return;
	}

	SetCombatState(EPBCombatState::TurnInProgress);
	BeginTurnForCurrentEntry();
}

void UPBCombatManagerSubsystem::BeginTurnForCurrentEntry()
{
	if (!InitiativeOrder.IsValidIndex(CurrentTurnIndex))
	{
		return;
	}

	const FPBInitiativeEntry& CurrentEntry = InitiativeOrder[CurrentTurnIndex];
	int32 GroupId = CurrentEntry.SharedTurnGroupId;

	if (GroupId >= 0)
	{
		// 공유 턴 그룹: 모든 멤버가 행동불능인지 확인
		TArray<AActor*> Group = GetCurrentSharedTurnGroup();
		bool bAllIncapacitated = true;
		for (AActor* Member : Group)
		{
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Member))
			{
				if (!Participant->IsIncapacitated())
				{
					bAllIncapacitated = false;
					break;
				}
			}
		}

		if (bAllIncapacitated)
		{
			// 그룹 전체 행동불능 → 스킵
			AdvanceToNextTurn();
			return;
		}

		// 모든 멤버에게 OnTurnBegin (리소스 리셋)
		// 첫 번째 비행동불능 멤버를 활성으로 설정하고 OnTurnActivated 호출
		SharedTurnActiveIndex = INDEX_NONE;
		GroupActedMemberIndices.Empty();
		for (int32 i = 0; i < Group.Num(); ++i)
		{
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Group[i]))
			{
				if (!Participant->IsIncapacitated())
				{
					Participant->OnTurnBegin();
					if (SharedTurnActiveIndex == INDEX_NONE)
					{
						SharedTurnActiveIndex = i;
					}
				}
			}
		}

		// 첫 번째 활성 멤버에게만 OnTurnActivated (AI 행동 트리거)
		if (Group.IsValidIndex(SharedTurnActiveIndex))
		{
			if (IPBCombatParticipant* ActiveParticipant = Cast<IPBCombatParticipant>(Group[SharedTurnActiveIndex]))
			{
				ActiveParticipant->OnTurnActivated();
			}
		}

		OnActiveTurnChanged.Broadcast(GetCurrentCombatant(), CurrentTurnIndex);
	}
	else
	{
		// 개별 턴
		AActor* Actor = CurrentEntry.Combatant.Get();
		IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Actor);

		if (!IsValid(Actor) || !Participant)
		{
			AdvanceToNextTurn();
			return;
		}

		if (Participant->IsIncapacitated())
		{
			// 행동불능 → 스킵
			AdvanceToNextTurn();
			return;
		}

		Participant->OnTurnBegin();
		Participant->OnTurnActivated();
		OnActiveTurnChanged.Broadcast(Actor, CurrentTurnIndex);
	}
}

void UPBCombatManagerSubsystem::HandleRoundEnd()
{
	SetCombatState(EPBCombatState::RoundEnding);

	// 전투 종료 조건 확인
	if (CheckCombatEndCondition())
	{
		EndCombat();
		return;
	}

	// 다음 라운드 시작
	CurrentRound++;
	CurrentTurnIndex = 0;
	SharedTurnActiveIndex = 0;
	GroupActedMemberIndices.Empty();
	OnRoundChanged.Broadcast(CurrentRound);

	// 모든 참가자에게 OnRoundBegin (Reaction 리필)
	for (const TWeakObjectPtr<AActor>& WeakCombatant : RegisteredCombatants)
	{
		if (AActor* Combatant = WeakCombatant.Get())
		{
			if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Combatant))
			{
				if (!Participant->IsIncapacitated())
				{
					Participant->OnRoundBegin();
				}
			}
		}
	}

	SetCombatState(EPBCombatState::TurnInProgress);
	BeginTurnForCurrentEntry();
}

bool UPBCombatManagerSubsystem::CheckCombatEndCondition() const
{
	// 진영별 생존자 수 확인
	TMap<FGameplayTag, int32> FactionSurvivors;

	for (const FPBInitiativeEntry& Entry : InitiativeOrder)
	{
		AActor* Actor = Entry.Combatant.Get();
		IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Actor);
		if (!IsValid(Actor) || !Participant)
		{
			continue;
		}

		if (!Participant->IsIncapacitated())
		{
			FGameplayTag Faction = Participant->GetFactionTag();
			if (Faction.IsValid())
			{
				FactionSurvivors.FindOrAdd(Faction)++;
			}
		}
	}

	// 중립 진영 제외, 생존 진영이 1개 이하이면 전투 종료
	int32 ActiveFactions = 0;
	for (const auto& Pair : FactionSurvivors)
	{
		if (Pair.Key != PBGameplayTags::Combat_Faction_Neutral && Pair.Value > 0)
		{
			ActiveFactions++;
		}
	}

	return ActiveFactions <= 1;
}

void UPBCombatManagerSubsystem::HandleIncapacitated(AActor* Combatant)
{
	// 현재 턴 소유자인 경우 즉시 턴 종료
	AActor* CurrentActor = GetCurrentCombatant();
	if (CurrentActor == Combatant && CombatState == EPBCombatState::TurnInProgress)
	{
		EndCurrentTurn();
		return;
	}

	// 전투 종료 조건 확인
	if (CheckCombatEndCondition())
	{
		EndCombat();
	}
}
