// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCombatCheatManager.h"
#include "PBTestCombatCharacter.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/Utils/PBDebugUtils.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/Game/PBGameplayGameMode.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"

// 화면 출력 Key 정의
namespace CombatCheatKeys
{
	// Combat.Status HUD (100~120)
	constexpr int32 StatusHeader       = 100;
	constexpr int32 StatusState        = 101;
	constexpr int32 StatusRound        = 102;
	constexpr int32 StatusTurnIndex    = 103;
	constexpr int32 StatusCurrentTurn  = 104;
	constexpr int32 StatusOrderHeader  = 105;
	constexpr int32 StatusOrderBase    = 106; // 106~115 (최대 10명)
	constexpr int32 StatusGroupHeader  = 116;
	constexpr int32 StatusGroupBase    = 117; // 117~119
	constexpr int32 StatusFooter       = 120;

	// Combat.Start / End (130~139)
	constexpr int32 CombatStart        = 130;
	constexpr int32 CombatEnd          = 131;

	// Combat.EndTurn (140~149)
	constexpr int32 EndTurn            = 140;

	// Combat.Kill (150~159)
	constexpr int32 Kill               = 150;

	// Combat.Reaction (160~169)
	constexpr int32 Reaction           = 160;
	constexpr int32 ResolveReaction    = 161;

	// Combat.SwitchMember (170~179)
	constexpr int32 SwitchMember       = 170;

	// Combat.Help (180~189)
	constexpr int32 HelpBase           = 180;

	// 경고/에러 (190~199)
	constexpr int32 Warning            = 190;
	constexpr int32 Error              = 191;
}

UPBCombatManagerSubsystem* UPBCombatCheatManager::GetCombatManager() const
{
	if (APlayerController* PC = GetOuterAPlayerController())
	{
		if (UWorld* World = PC->GetWorld())
		{
			return World->GetSubsystem<UPBCombatManagerSubsystem>();
		}
	}
	return nullptr;
}

void UPBCombatCheatManager::SpawnTestEnemies(int32 NumEnemies)
{
	CleanupTestCharacters();

	APlayerController* PC = GetOuterAPlayerController();
	if (!IsValid(PC))
	{
		return;
	}

	UWorld* World = PC->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	FVector SpawnOrigin = FVector::ZeroVector;
	
	if (auto Pawn = GetPlayerController()->GetPawn())
	{
		SpawnOrigin = Pawn->GetActorLocation();
	}
	
	// 적 스폰
	for (int32 i = 0; i < NumEnemies; ++i)
	{
		FVector SpawnLoc = SpawnOrigin + FVector(i * 300.0f, 300.0f, 10.0f);
		APBTestCombatCharacter* Enemy = World->SpawnActor<APBTestCombatCharacter>(
			AICharacterClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);

		if (IsValid(Enemy))
		{
			Enemy->TestInitiativeModifier = FMath::RandRange(0, 3);
			SpawnedEnemies.Add(Enemy);
		}
	}
}

void UPBCombatCheatManager::CleanupTestCharacters()
{
	for (TObjectPtr<APBTestCombatCharacter>& Character : SpawnedEnemies)
	{
		if (IsValid(Character))
		{
			Character->Destroy();
		}
	}
	SpawnedEnemies.Empty();
}

void UPBCombatCheatManager::RefreshStatusHUD()
{
	if (!bStatusHUDVisible)
	{
		return;
	}

	UPBCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (!IsValid(CombatManager))
	{
		return;
	}

	static const TCHAR* StateNames[] = {
		TEXT("OutOfCombat"), TEXT("CombatStarting"), TEXT("InitiativeRoll"),
		TEXT("TurnInProgress"), TEXT("WaitingForReaction"), TEXT("TurnEnding"),
		TEXT("RoundEnding"), TEXT("CombatEnding")
	};

	EPBCombatState State = CombatManager->GetCombatState();
	int32 StateIndex = static_cast<int32>(State);

	DebugUtils::PrintOnScreen(TEXT("──────────── 전투 상태 ────────────"),
		CombatCheatKeys::StatusHeader, 999999.0f, FColor::Cyan);
	DebugUtils::PrintOnScreen(FString::Printf(TEXT("  상태: %s"), StateNames[StateIndex]),
		CombatCheatKeys::StatusState, 999999.0f, FColor::White);
	DebugUtils::PrintOnScreen(FString::Printf(TEXT("  라운드: %d"), CombatManager->GetCurrentRound()),
		CombatCheatKeys::StatusRound, 999999.0f, FColor::White);
	DebugUtils::PrintOnScreen(FString::Printf(TEXT("  턴 인덱스: %d"), CombatManager->GetCurrentTurnIndex()),
		CombatCheatKeys::StatusTurnIndex, 999999.0f, FColor::White);

	if (AActor* CurrentActor = CombatManager->GetCurrentCombatant())
	{
		if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(CurrentActor))
		{
			DebugUtils::PrintOnScreen(FString::Printf(TEXT("  현재 턴: %s"),
				*Participant->GetCombatDisplayName().ToString()),
				CombatCheatKeys::StatusCurrentTurn, 999999.0f, FColor::Green);
		}
	}
	else
	{
		DebugUtils::PrintOnScreen(TEXT("  현재 턴: -"),
			CombatCheatKeys::StatusCurrentTurn, 999999.0f, FColor::Silver);
	}

	const TArray<FPBInitiativeEntry>& Order = CombatManager->GetInitiativeOrder();
	DebugUtils::PrintOnScreen(TEXT("  ── 이니셔티브 순서 ──"),
		CombatCheatKeys::StatusOrderHeader, 999999.0f, FColor::Cyan);

	for (int32 i = 0; i < 10; ++i)
	{
		if (i < Order.Num())
		{
			const FPBInitiativeEntry& Entry = Order[i];
			FString Name = TEXT("???");
			FString FactionStr = TEXT("?");
			bool bIncap = false;

			if (AActor* Actor = Entry.Combatant.Get())
			{
				if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Actor))
				{
					Name = Participant->GetCombatDisplayName().ToString();
					FactionStr = Participant->GetFactionTag().ToString();
					bIncap = Participant->IsIncapacitated();
				}
			}

			FColor EntryColor = (i == CombatManager->GetCurrentTurnIndex()) ? FColor::Green : FColor::Silver;
			if (bIncap)
			{
				EntryColor = FColor::Red;
			}

			DebugUtils::PrintOnScreen(
				FString::Printf(TEXT("  [%d] %s | Init:%d (Roll:%d + Mod:%d) | 진영:%s | 그룹:%d%s"),
					i, *Name, Entry.InitiativeTotal, Entry.RawRoll, Entry.DexModifier,
					*FactionStr, Entry.SharedTurnGroupId,
					bIncap ? TEXT(" [행동불능]") : TEXT("")),
				CombatCheatKeys::StatusOrderBase + i, -1.0f, EntryColor);
		}
		else
		{
			// 이전 표시 제거 (빈 문자열로 덮어쓰기)
			DebugUtils::PrintOnScreen(TEXT(""),
				CombatCheatKeys::StatusOrderBase + i, 0.01f, FColor::Black);
		}
	}

	TArray<AActor*> Group = CombatManager->GetCurrentSharedTurnGroup();
	if (Group.Num() > 1)
	{
		DebugUtils::PrintOnScreen(FString::Printf(TEXT("  ── 현재 공유 턴 그룹 (%d명) ──"), Group.Num()),
			CombatCheatKeys::StatusGroupHeader, 999999.0f, FColor::Cyan);

		int32 GroupIdx = 0;
		for (AActor* Member : Group)
		{
			if (IPBCombatParticipant* P = Cast<IPBCombatParticipant>(Member))
			{
				if (GroupIdx < 3)
				{
					DebugUtils::PrintOnScreen(
						FString::Printf(TEXT("    - %s%s"),
							*P->GetCombatDisplayName().ToString(),
							Member == CombatManager->GetCurrentCombatant() ? TEXT(" [활성]") : TEXT("")),
						CombatCheatKeys::StatusGroupBase + GroupIdx, 999999.0f, FColor::White);
				}
				++GroupIdx;
			}
		}
		// 나머지 그룹 슬롯 제거
		for (int32 i = GroupIdx; i < 3; ++i)
		{
			DebugUtils::PrintOnScreen(TEXT(""),
				CombatCheatKeys::StatusGroupBase + i, 0.01f, FColor::Black);
		}
	}
	else
	{
		// 공유 턴 그룹 없으면 해당 영역 제거
		DebugUtils::PrintOnScreen(TEXT(""),
			CombatCheatKeys::StatusGroupHeader, 0.01f, FColor::Black);
		for (int32 i = 0; i < 3; ++i)
		{
			DebugUtils::PrintOnScreen(TEXT(""),
				CombatCheatKeys::StatusGroupBase + i, 0.01f, FColor::Black);
		}
	}

	DebugUtils::PrintOnScreen(TEXT("────────────────────────────────────"),
		CombatCheatKeys::StatusFooter, 999999.0f, FColor::Cyan);
}

void UPBCombatCheatManager::ClearStatusHUD()
{
	// 모든 Status Key를 짧은 시간으로 빈 문자열 출력하여 제거
	for (int32 Key = CombatCheatKeys::StatusHeader; Key <= CombatCheatKeys::StatusFooter; ++Key)
	{
		DebugUtils::PrintOnScreen(TEXT(""), Key, 0.01f, FColor::Black);
	}
}

void UPBCombatCheatManager::Combat_Start(int32 NumEnemies)
{
	UPBCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (!IsValid(CombatManager))
	{
		DebugUtils::Print(TEXT("[Combat.Start] CombatManagerSubsystem을 찾을 수 없습니다."),
			true, FColor::Red, CombatCheatKeys::Error);
		return;
	}

	if (CombatManager->IsInCombat())
	{
		DebugUtils::Print(TEXT("[Combat.Start] 이미 전투 중입니다. Combat.End로 먼저 종료하세요."),
			true, FColor::Yellow, CombatCheatKeys::Warning);
		return;
	}

	TArray<AActor*> PartyMembers;
	if (APBGameplayPlayerState* PS = GetPlayerController()->GetPlayerState<APBGameplayPlayerState>())
	{
		PartyMembers = PS->GetPartyMembers();
	}
	
	NumEnemies = FMath::Max(1, NumEnemies);
	SpawnTestEnemies(NumEnemies);

	TArray<AActor*> Combatants;
	for (AActor* PartyMember : PartyMembers)
	{
		Combatants.Add(PartyMember);
	}
	for (APBTestCombatCharacter* Character : SpawnedEnemies)
	{
		Combatants.Add(Character);
	}
	
	if (APBGameplayGameMode* GM = GetWorld()->GetAuthGameMode<APBGameplayGameMode>())
	{
		DebugUtils::Print(FString::Printf(TEXT("[Combat.Start] ===== 전투 시작 (아군 %d, 적 %d) ====="), PartyMembers.Num(), NumEnemies),
		true, FColor::Cyan, CombatCheatKeys::CombatStart);
		
		GM->InitiateCombat(Combatants);
		bStatusHUDVisible = true;
		RefreshStatusHUD();
	}
}

void UPBCombatCheatManager::Combat_EndTurn()
{
	UPBCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (!IsValid(CombatManager) || !CombatManager->IsInCombat())
	{
		DebugUtils::Print(TEXT("[Combat.EndTurn] 전투 중이 아닙니다."),
			true, FColor::Yellow, CombatCheatKeys::Warning);
		return;
	}

	AActor* Current = CombatManager->GetCurrentCombatant();
	FString CurrentName = TEXT("Unknown");
	if (IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Current))
	{
		CurrentName = Participant->GetCombatDisplayName().ToString();
	}

	DebugUtils::Print(FString::Printf(TEXT("[Combat.EndTurn] [%s]의 턴 종료"), *CurrentName),
		true, FColor::White, CombatCheatKeys::EndTurn);
	CombatManager->EndCurrentTurn();

	RefreshStatusHUD();
}

void UPBCombatCheatManager::Combat_End()
{
	UPBCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (!IsValid(CombatManager) || !CombatManager->IsInCombat())
	{
		DebugUtils::Print(TEXT("[Combat.End] 전투 중이 아닙니다."),
			true, FColor::Yellow, CombatCheatKeys::Warning);
		return;
	}

	DebugUtils::Print(TEXT("[Combat.End] ===== 전투 종료 ====="),
		true, FColor::Cyan, CombatCheatKeys::CombatEnd);
	CombatManager->EndCombat();
	CleanupTestCharacters();
	bStatusHUDVisible = false;
	ClearStatusHUD();
}

void UPBCombatCheatManager::Combat_Kill()
{
	UPBCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (!IsValid(CombatManager) || !CombatManager->IsInCombat())
	{
		DebugUtils::Print(TEXT("[Combat.Kill] 전투 중이 아닙니다."),
			true, FColor::Yellow, CombatCheatKeys::Warning);
		return;
	}

	for (APBTestCombatCharacter* Character : SpawnedEnemies)
	{
		if (IsValid(Character) && Character->GetFactionTag() == PBGameplayTags::Combat_Faction_Enemy
			&& !Character->IsIncapacitated())
		{
			DebugUtils::Print(FString::Printf(TEXT("[Combat.Kill] [%s] 행동불능 처리"),
				*Character->GetCombatDisplayName().ToString()),
				true, FColor::Orange, CombatCheatKeys::Kill);
			Character->SetIncapacitated(true);
			CombatManager->NotifyCombatantIncapacitated(Character);
			RefreshStatusHUD();
			return;
		}
	}

	DebugUtils::Print(TEXT("[Combat.Kill] 행동불능 처리할 적 캐릭터가 없습니다."),
		true, FColor::Yellow, CombatCheatKeys::Warning);
}

void UPBCombatCheatManager::Combat_Reaction()
{
	UPBCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (!IsValid(CombatManager) || !CombatManager->IsInCombat())
	{
		DebugUtils::Print(TEXT("[Combat.Reaction] 전투 중이 아닙니다."),
			true, FColor::Yellow, CombatCheatKeys::Warning);
		return;
	}

	AActor* CurrentActor = CombatManager->GetCurrentCombatant();
	if (!IsValid(CurrentActor))
	{
		return;
	}

	IPBCombatParticipant* CurrentParticipant = Cast<IPBCombatParticipant>(CurrentActor);
	if (!CurrentParticipant)
	{
		return;
	}

	FGameplayTag CurrentFaction = CurrentParticipant->GetFactionTag();

	for (APBTestCombatCharacter* Character : SpawnedEnemies)
	{
		if (!IsValid(Character) || Character == CurrentActor)
		{
			continue;
		}

		IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Character);
		if (!Participant || Participant->GetFactionTag() == CurrentFaction)
		{
			continue;
		}

		if (Participant->CanReact())
		{
			FPBReactionContext Context;
			Context.Trigger = EPBReactionTrigger::LeavesMeleeRange;
			Context.TriggeringActor = CurrentActor;
			Context.ReactingActor = Character;

			DebugUtils::Print(FString::Printf(TEXT("[Combat.Reaction] [%s]의 이동 → [%s]에게 반응 기회"),
				*CurrentParticipant->GetCombatDisplayName().ToString(),
				*Participant->GetCombatDisplayName().ToString()),
				true, FColor::Magenta, CombatCheatKeys::Reaction);

			CombatManager->TriggerReaction(Context);
			RefreshStatusHUD();
			return;
		}
	}

	DebugUtils::Print(TEXT("[Combat.Reaction] 반응 가능한 상대 진영 캐릭터가 없습니다."),
		true, FColor::Yellow, CombatCheatKeys::Warning);
}

void UPBCombatCheatManager::Combat_ResolveReaction(int32 bUse)
{
	UPBCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (!IsValid(CombatManager))
	{
		return;
	}

	if (CombatManager->GetCombatState() != EPBCombatState::WaitingForReaction)
	{
		DebugUtils::Print(TEXT("[Combat.ResolveReaction] 반응 대기 중이 아닙니다."),
			true, FColor::Yellow, CombatCheatKeys::Warning);
		return;
	}

	bool bUsed = (bUse != 0);
	DebugUtils::Print(FString::Printf(TEXT("[Combat.ResolveReaction] 반응 %s"), bUsed ? TEXT("사용") : TEXT("거부")),
		true, FColor::Magenta, CombatCheatKeys::ResolveReaction);
	CombatManager->ResolveReaction(bUsed);
	RefreshStatusHUD();
}

void UPBCombatCheatManager::Combat_SwitchMember()
{
	UPBCombatManagerSubsystem* CombatManager = GetCombatManager();
	if (!IsValid(CombatManager) || !CombatManager->IsInCombat())
	{
		DebugUtils::Print(TEXT("[Combat.SwitchMember] 전투 중이 아닙니다."),
			true, FColor::Yellow, CombatCheatKeys::Warning);
		return;
	}

	TArray<AActor*> Group = CombatManager->GetCurrentSharedTurnGroup();
	if (Group.Num() <= 1)
	{
		DebugUtils::Print(TEXT("[Combat.SwitchMember] 현재 공유 턴 그룹이 아니거나 멤버가 1명입니다."),
			true, FColor::Yellow, CombatCheatKeys::Warning);
		return;
	}

	AActor* CurrentActive = CombatManager->GetCurrentCombatant();
	for (AActor* Member : Group)
	{
		if (Member != CurrentActive)
		{
			if (IPBCombatParticipant* P = Cast<IPBCombatParticipant>(Member))
			{
				if (!P->IsIncapacitated())
				{
					DebugUtils::Print(FString::Printf(TEXT("[Combat.SwitchMember] 공유 턴 전환 → [%s]"),
						*P->GetCombatDisplayName().ToString()),
						true, FColor::Green, CombatCheatKeys::SwitchMember);
					CombatManager->SwitchToGroupMember(Member);
					RefreshStatusHUD();
					return;
				}
			}
		}
	}

	DebugUtils::Print(TEXT("[Combat.SwitchMember] 전환 가능한 멤버가 없습니다."),
		true, FColor::Yellow, CombatCheatKeys::Warning);
}

void UPBCombatCheatManager::Combat_Status(int32 bShow)
{
	if (bShow != 0)
	{
		bStatusHUDVisible = true;
		RefreshStatusHUD();
	}
	else
	{
		bStatusHUDVisible = false;
		ClearStatusHUD();
	}
}

void UPBCombatCheatManager::Combat_Help()
{
	const TArray<FString> HelpLines = {
		TEXT("──────────── Combat Cheat 명령어 ────────────"),
		TEXT("  Combat_Start [아군수] [적수]    - 더미 캐릭터 스폰 + 전투 시작"),
		TEXT("  Combat_EndTurn                  - 현재 턴 종료"),
		TEXT("  Combat_End                      - 전투 강제 종료"),
		TEXT("  Combat_Kill                     - 첫 번째 생존 적 행동불능"),
		TEXT("  Combat_Reaction                 - 반응 트리거 테스트"),
		TEXT("  Combat_ResolveReaction [0/1]    - 반응 해결 (0=거부, 1=사용)"),
		TEXT("  Combat_SwitchMember             - 공유 턴 그룹 내 전환"),
		TEXT("  Combat_Status [0/1]             - 전투 상태 HUD (1=표시, 0=제거)"),
		TEXT("  Combat_Help                     - 이 도움말 표시"),
		TEXT("─────────────────────────────────────────────"),
	};

	for (int32 i = 0; i < HelpLines.Num(); ++i)
	{
		DebugUtils::Print(HelpLines[i], true, FColor::Cyan, CombatCheatKeys::HelpBase + i);
	}
}
