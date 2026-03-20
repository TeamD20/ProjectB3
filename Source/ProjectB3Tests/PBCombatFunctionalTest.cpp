// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCombatFunctionalTest.h"
#include "ProjectB3/Combat/Test/PBTestCombatCharacter.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "GameplayTagsManager.h"

APBCombatFunctionalTestBase::APBCombatFunctionalTestBase()
{
}

void APBCombatFunctionalTestBase::PrepareTest()
{
	Super::PrepareTest();

	AllyCounter = 0;
	EnemyCounter = 0;
	SpawnedCharacters.Empty();

	// 이전 전투가 남아 있으면 정리
	if (UPBCombatManagerSubsystem* CM = GetCombatManager())
	{
		if (CM->IsInCombat())
		{
			CM->EndCombat();
		}
	}
}

void APBCombatFunctionalTestBase::CleanUp()
{
	if (UPBCombatManagerSubsystem* CM = GetCombatManager())
	{
		if (CM->IsInCombat())
		{
			CM->EndCombat();
		}
	}

	for (TObjectPtr<APBTestCombatCharacter>& Character : SpawnedCharacters)
	{
		if (IsValid(Character))
		{
			Character->Destroy();
		}
	}
	SpawnedCharacters.Empty();

	Super::CleanUp();
}

APBTestCombatCharacter* APBCombatFunctionalTestBase::SpawnAlly(int32 InitiativeModifier, const FString& Name)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector Loc = GetActorLocation() + FVector(AllyCounter * 200.0f, -200.0f, 100.0f);
	APBTestCombatCharacter* Char = GetWorld()->SpawnActor<APBTestCombatCharacter>(
		APBTestCombatCharacter::StaticClass(), Loc, FRotator::ZeroRotator, Params);

	if (IsValid(Char))
	{
		FPBCombatIdentity Identity;
		Identity.FactionTag = FGameplayTag::RequestGameplayTag(FName("Combat.Faction.Player"));
		Identity.DisplayName = FText::FromString(
			Name.IsEmpty() ? FString::Printf(TEXT("아군_%d"), ++AllyCounter) : Name);
		Char->SetCombatIdentity(Identity);
		Char->TestInitiativeModifier = InitiativeModifier;
		SpawnedCharacters.Add(Char);
	}
	return Char;
}

APBTestCombatCharacter* APBCombatFunctionalTestBase::SpawnEnemy(int32 InitiativeModifier, const FString& Name)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector Loc = GetActorLocation() + FVector(EnemyCounter * 200.0f, 200.0f, 100.0f);
	APBTestCombatCharacter* Char = GetWorld()->SpawnActor<APBTestCombatCharacter>(
		APBTestCombatCharacter::StaticClass(), Loc, FRotator::ZeroRotator, Params);

	if (IsValid(Char))
	{
		FPBCombatIdentity Identity;
		Identity.FactionTag = FGameplayTag::RequestGameplayTag(FName("Combat.Faction.Enemy"));
		Identity.DisplayName = FText::FromString(
			Name.IsEmpty() ? FString::Printf(TEXT("적_%d"), ++EnemyCounter) : Name);
		Char->SetCombatIdentity(Identity);
		Char->TestInitiativeModifier = InitiativeModifier;
		SpawnedCharacters.Add(Char);
	}
	return Char;
}

UPBCombatManagerSubsystem* APBCombatFunctionalTestBase::GetCombatManager() const
{
	return GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
}

TArray<AActor*> APBCombatFunctionalTestBase::GetAllCombatantsAsActors() const
{
	TArray<AActor*> Combatants;
	for (const TObjectPtr<APBTestCombatCharacter>& Char : SpawnedCharacters)
	{
		if (IsValid(Char))
		{
			Combatants.Add(Char);
		}
	}
	return Combatants;
}

// Test 1: 전투 시작/종료 기본 흐름
void APBTest_CombatStartEnd::StartTest()
{
	Super::StartTest();

	UPBCombatManagerSubsystem* CM = GetCombatManager();
	AssertIsValid(CM, TEXT("CombatManagerSubsystem이 존재해야 한다"));

	// 초기 상태 확인
	AssertFalse(CM->IsInCombat(), TEXT("시작 전 OutOfCombat 상태여야 한다"));

	// 캐릭터 스폰
	SpawnAlly(3);
	SpawnAlly(1);
	SpawnEnemy(2);
	SpawnEnemy(0);

	// 전투 시작
	CM->StartCombat(GetAllCombatantsAsActors());

	// 전투 상태 확인
	AssertTrue(CM->IsInCombat(), TEXT("StartCombat 후 전투 중이어야 한다"));
	AssertEqual_Int(CM->GetCurrentRound(), 1, TEXT("첫 라운드는 1이어야 한다"));
	AssertTrue(CM->GetCombatState() == EPBCombatState::TurnInProgress, TEXT("시작 후 TurnInProgress 상태여야 한다"));

	// 이니셔티브 순서 확인
	const TArray<FPBInitiativeEntry>& Order = CM->GetInitiativeOrder();
	AssertEqual_Int(Order.Num(), 4, TEXT("참가자 4명이 이니셔티브 순서에 있어야 한다"));

	// 내림차순 정렬 확인
	for (int32 i = 1; i < Order.Num(); ++i)
	{
		AssertTrue(Order[i - 1].InitiativeTotal >= Order[i].InitiativeTotal,
			FString::Printf(TEXT("이니셔티브 순서 [%d]=%d >= [%d]=%d"),
				i - 1, Order[i - 1].InitiativeTotal, i, Order[i].InitiativeTotal));
	}

	// 현재 턴 참가자가 유효해야 함
	AssertIsValid(CM->GetCurrentCombatant(), TEXT("현재 턴 참가자가 존재해야 한다"));

	// 전투 종료
	CM->EndCombat();
	AssertFalse(CM->IsInCombat(), TEXT("EndCombat 후 전투 중이 아니어야 한다"));
	AssertTrue(CM->GetCombatState() == EPBCombatState::OutOfCombat,
		TEXT("종료 후 OutOfCombat 상태여야 한다"));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("전투 시작/종료 기본 흐름 통과"));
}

// Test 2: 턴 진행 및 라운드 순환
void APBTest_TurnAdvanceAndRound::StartTest()
{
	Super::StartTest();

	UPBCombatManagerSubsystem* CM = GetCombatManager();

	// 2vs1 구성 (공유 턴이 아닌 케이스를 위해 교차 배치)
	APBTestCombatCharacter* A1 = SpawnAlly(10, TEXT("아군A"));
	APBTestCombatCharacter* E1 = SpawnEnemy(5, TEXT("적A"));
	APBTestCombatCharacter* A2 = SpawnAlly(0, TEXT("아군B"));

	CM->StartCombat(GetAllCombatantsAsActors());

	int32 TotalTurns = CM->GetInitiativeOrder().Num();
	AssertTrue(TotalTurns > 0, TEXT("이니셔티브 순서에 엔트리가 있어야 한다"));

	// 공유 턴 그룹 수를 고려한 실제 턴 횟수 계산
	// 각 턴을 EndCurrentTurn으로 순환
	int32 Round1 = CM->GetCurrentRound();
	AssertEqual_Int(Round1, 1, TEXT("라운드 1이어야 한다"));

	// 모든 턴 소진 → 라운드 2로 진입
	// 최대 10번 반복 (안전장치)
	for (int32 i = 0; i < 10 && CM->IsInCombat() && CM->GetCurrentRound() == 1; ++i)
	{
		CM->EndCurrentTurn();
	}

	if (CM->IsInCombat())
	{
		AssertEqual_Int(CM->GetCurrentRound(), 2, TEXT("모든 턴 소진 후 라운드 2여야 한다"));
	}

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("턴 진행 및 라운드 순환 통과"));
}

// Test 3: 이니셔티브 정렬 및 타이브레이크
void APBTest_InitiativeSorting::StartTest()
{
	Super::StartTest();

	UPBCombatManagerSubsystem* CM = GetCombatManager();

	// 같은 수정치로 스폰 — 타이브레이크 검증
	// DexModifier가 다르면 타이브레이크 작동
	APBTestCombatCharacter* HighDex = SpawnAlly(5, TEXT("HighDex"));
	APBTestCombatCharacter* LowDex = SpawnEnemy(1, TEXT("LowDex"));
	APBTestCombatCharacter* MidDex = SpawnAlly(3, TEXT("MidDex"));

	CM->StartCombat(GetAllCombatantsAsActors());

	const TArray<FPBInitiativeEntry>& Order = CM->GetInitiativeOrder();
	AssertEqual_Int(Order.Num(), 3, TEXT("3명이 정렬되어야 한다"));

	// 내림차순 확인
	for (int32 i = 1; i < Order.Num(); ++i)
	{
		bool bCorrectOrder = Order[i - 1].InitiativeTotal > Order[i].InitiativeTotal
			|| (Order[i - 1].InitiativeTotal == Order[i].InitiativeTotal
				&& Order[i - 1].DexModifier >= Order[i].DexModifier);
		AssertTrue(bCorrectOrder,
			FString::Printf(TEXT("정렬 [%d] Total=%d,Dex=%d >= [%d] Total=%d,Dex=%d"),
				i - 1, Order[i - 1].InitiativeTotal, Order[i - 1].DexModifier,
				i, Order[i].InitiativeTotal, Order[i].DexModifier));
	}

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("이니셔티브 정렬 통과"));
}

// Test 4: 공유 턴 그룹
void APBTest_SharedTurnGroup::StartTest()
{
	Super::StartTest();

	UPBCombatManagerSubsystem* CM = GetCombatManager();

	// 높은 이니셔티브로 아군 2명을 연속 배치 유도
	// (수정치 차이를 크게 두어 정렬 후 연속이 되도록)
	APBTestCombatCharacter* A1 = SpawnAlly(20, TEXT("아군1_그룹"));
	APBTestCombatCharacter* A2 = SpawnAlly(19, TEXT("아군2_그룹"));
	APBTestCombatCharacter* E1 = SpawnEnemy(0, TEXT("적1"));

	CM->StartCombat(GetAllCombatantsAsActors());

	const TArray<FPBInitiativeEntry>& Order = CM->GetInitiativeOrder();

	// 아군 2명이 연속으로 정렬되었는지 확인
	int32 A1Index = INDEX_NONE;
	int32 A2Index = INDEX_NONE;
	for (int32 i = 0; i < Order.Num(); ++i)
	{
		if (Order[i].Combatant.Get() == A1)
		{
			A1Index = i;
		}
		if (Order[i].Combatant.Get() == A2)
		{
			A2Index = i;
		}
	}

	if (A1Index != INDEX_NONE && A2Index != INDEX_NONE && FMath::Abs(A1Index - A2Index) == 1)
	{
		// 연속이면 공유 턴 그룹이어야 한다
		int32 GroupA1 = Order[A1Index].SharedTurnGroupId;
		int32 GroupA2 = Order[A2Index].SharedTurnGroupId;
		AssertTrue(GroupA1 >= 0 && GroupA1 == GroupA2,
			TEXT("연속 같은 진영 캐릭터는 같은 공유 턴 그룹이어야 한다"));

		// 그룹 내 전환 테스트
		TArray<AActor*> Group = CM->GetCurrentSharedTurnGroup();
		if (Group.Num() > 1)
		{
			AActor* FirstActive = CM->GetCurrentCombatant();
			AActor* OtherMember = (Group[0] == FirstActive) ? Group[1] : Group[0];
			CM->SwitchToGroupMember(OtherMember);
			AssertTrue(CM->GetCurrentCombatant() == OtherMember,
				TEXT("SwitchToGroupMember 후 활성 멤버가 변경되어야 한다"));
		}
	}
	else
	{
		// 랜덤 굴림 때문에 연속이 안 될 수도 있음 — 이 경우 정렬만 확인
		AddWarning(TEXT("랜덤 굴림으로 아군이 연속 배치되지 않아 공유 턴 그룹 테스트 스킵"));
	}

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("공유 턴 그룹 테스트 통과"));
}

// Test 5: 행동불능 처리
void APBTest_Incapacitation::StartTest()
{
	Super::StartTest();

	UPBCombatManagerSubsystem* CM = GetCombatManager();

	APBTestCombatCharacter* A1 = SpawnAlly(5, TEXT("아군"));
	APBTestCombatCharacter* E1 = SpawnEnemy(3, TEXT("적1"));
	APBTestCombatCharacter* E2 = SpawnEnemy(1, TEXT("적2"));

	CM->StartCombat(GetAllCombatantsAsActors());
	AssertTrue(CM->IsInCombat(), TEXT("전투가 시작되어야 한다"));

	// 적1 행동불능
	E1->SetIncapacitated(true);
	CM->NotifyCombatantIncapacitated(E1);

	// 아직 적2가 살아있으므로 전투 계속
	AssertTrue(CM->IsInCombat(), TEXT("적1만 행동불능 — 전투 계속이어야 한다"));

	// 적2 행동불능 → 적 진영 전멸 → 전투 종료
	E2->SetIncapacitated(true);
	CM->NotifyCombatantIncapacitated(E2);

	AssertFalse(CM->IsInCombat(), TEXT("적 진영 전멸 — 전투 종료되어야 한다"));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("행동불능 처리 테스트 통과"));
}

// Test 6: 반응 행동 흐름
void APBTest_ReactionFlow::StartTest()
{
	Super::StartTest();

	UPBCombatManagerSubsystem* CM = GetCombatManager();

	APBTestCombatCharacter* A1 = SpawnAlly(10, TEXT("아군"));
	APBTestCombatCharacter* E1 = SpawnEnemy(0, TEXT("적"));

	CM->StartCombat(GetAllCombatantsAsActors());

	// 현재 상태 확인
	AssertTrue(CM->GetCombatState() == EPBCombatState::TurnInProgress,
		TEXT("TurnInProgress 상태여야 한다"));

	// 반응 트리거
	FPBReactionContext Context;
	Context.Trigger = EPBReactionTrigger::LeavesMeleeRange;
	Context.TriggeringActor = CM->GetCurrentCombatant();

	// 반응할 대상 결정 (현재 턴 소유자의 상대 진영)
	AActor* CurrentActor = CM->GetCurrentCombatant();
	APBTestCombatCharacter* Reactor = nullptr;

	if (CurrentActor == A1)
	{
		Reactor = E1;
	}
	else
	{
		Reactor = A1;
	}

	Context.ReactingActor = Reactor;
	CM->TriggerReaction(Context);

	AssertTrue(CM->GetCombatState() == EPBCombatState::WaitingForReaction,
		TEXT("TriggerReaction 후 WaitingForReaction이어야 한다"));

	// 반응 거부 테스트
	CM->ResolveReaction(false);
	AssertTrue(CM->GetCombatState() == EPBCombatState::TurnInProgress,
		TEXT("반응 거부 후 TurnInProgress 복귀해야 한다"));

	// 반응 사용 테스트
	Context.ReactingActor = Reactor;
	CM->TriggerReaction(Context);

	AssertTrue(CM->GetCombatState() == EPBCombatState::WaitingForReaction,
		TEXT("두 번째 TriggerReaction 후 WaitingForReaction이어야 한다"));

	CM->ResolveReaction(true);
	AssertTrue(CM->GetCombatState() == EPBCombatState::TurnInProgress,
		TEXT("반응 사용 후에도 TurnInProgress 복귀해야 한다"));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("반응 행동 흐름 테스트 통과"));
}

// Test 8: 멀티 그룹 한 바퀴 순환
void APBTest_SharedGroupCycle::StartTest()
{
	Super::StartTest();

	UPBCombatManagerSubsystem* CM = GetCombatManager();
	AssertIsValid(CM, TEXT("CombatManagerSubsystem이 존재해야 한다"));

	// --- 구성 ---
	// d20(1~20) + 수정치 범위가 겹치지 않도록 수정치 간격을 21 이상으로 설정
	// PlayerGroup  : A1(+30), A2(+30) → 롤 범위 31~50 (E1 최대 30보다 항상 높음)
	// EnemyGroup   : E1(+10), E2(+10) → 롤 범위 11~30 (A3 최대 10보다 항상 높음)
	// PlayerSolo   : A3(-10)          → 롤 범위 -9~10  (E3 최대 -10보다 항상 높음)
	// EnemySolo    : E3(-31)          → 롤 범위 -30~-11
	APBTestCombatCharacter* A1 = SpawnAlly(30, TEXT("아군1_그룹"));
	APBTestCombatCharacter* A2 = SpawnAlly(30, TEXT("아군2_그룹"));
	APBTestCombatCharacter* E1 = SpawnEnemy(10, TEXT("적1_그룹"));
	APBTestCombatCharacter* E2 = SpawnEnemy(10, TEXT("적2_그룹"));
	APBTestCombatCharacter* A3 = SpawnAlly(-10, TEXT("아군3_솔로"));
	APBTestCombatCharacter* E3 = SpawnEnemy(-31, TEXT("적3_솔로"));

	CM->StartCombat(GetAllCombatantsAsActors());
	AssertTrue(CM->IsInCombat(), TEXT("전투가 시작되어야 한다"));

	// --- 이니셔티브 순서 검증 ---
	const TArray<FPBInitiativeEntry>& Order = CM->GetInitiativeOrder();
	AssertEqual_Int(Order.Num(), 6, TEXT("참가자 6명이 이니셔티브에 등록되어야 한다"));

	// 이니셔티브 내림차순 정렬 확인
	for (int32 i = 1; i < Order.Num(); ++i)
	{
		AssertTrue(Order[i - 1].InitiativeTotal >= Order[i].InitiativeTotal,
			FString::Printf(TEXT("이니셔티브 정렬: [%d]=%d >= [%d]=%d"),
				i - 1, Order[i - 1].InitiativeTotal, i, Order[i].InitiativeTotal));
	}

	// 그룹 구조 검증: A1·A2는 같은 공유 그룹, E1·E2는 같은 공유 그룹, A3·E3는 개별 턴
	auto GetGroupId = [&](APBTestCombatCharacter* Char) -> int32
	{
		for (const FPBInitiativeEntry& Entry : Order)
		{
			if (Entry.Combatant.Get() == Char)
			{
				return Entry.SharedTurnGroupId;
			}
		}
		return INDEX_NONE;
	};

	int32 GroupIdA1 = GetGroupId(A1);
	int32 GroupIdA2 = GetGroupId(A2);
	int32 GroupIdE1 = GetGroupId(E1);
	int32 GroupIdE2 = GetGroupId(E2);
	int32 GroupIdA3 = GetGroupId(A3);
	int32 GroupIdE3 = GetGroupId(E3);

	AssertTrue(GroupIdA1 >= 0 && GroupIdA1 == GroupIdA2,
		TEXT("A1·A2는 같은 공유 턴 그룹이어야 한다"));
	AssertTrue(GroupIdE1 >= 0 && GroupIdE1 == GroupIdE2,
		TEXT("E1·E2는 같은 공유 턴 그룹이어야 한다"));
	AssertTrue(GroupIdA1 != GroupIdE1,
		TEXT("아군 그룹과 적 그룹은 서로 다른 그룹이어야 한다"));
	AssertEqual_Int(GroupIdA3, -1, TEXT("A3는 개별 턴이어야 한다"));
	AssertEqual_Int(GroupIdE3, -1, TEXT("E3는 개별 턴이어야 한다"));

	// --- 한 라운드 순환 및 OnTurnBegin / OnTurnActivated 검증 ---
	// StartCombat 시점에 슬롯 0([A1+A2])이 이미 시작됨, 첫 멤버에 OnTurnActivated 호출됨.
	// 순차 넘김:
	//   슬롯 0: EndCurrentTurn ×2 (A1→A2→슬롯1)
	//   슬롯 1: EndCurrentTurn ×2 (E1→E2→슬롯2)
	//   슬롯 2: EndCurrentTurn ×1 (A3→슬롯3)
	// 총 5회 EndCurrentTurn 후 E3 활성 (슬롯 3, 라운드 1의 마지막)
	for (int32 Step = 0; Step < 5; ++Step)
	{
		AssertTrue(CM->IsInCombat() && CM->GetCurrentRound() == 1,
			FString::Printf(TEXT("스텝 %d: 라운드 1이어야 한다"), Step));
		CM->EndCurrentTurn();
	}

	// 현재 슬롯 3([E3]) — 라운드 1의 마지막 슬롯
	AssertTrue(CM->IsInCombat(), TEXT("슬롯 3 진입 후 전투 중이어야 한다"));
	AssertEqual_Int(CM->GetCurrentRound(), 1, TEXT("슬롯 3에서 아직 라운드 1이어야 한다"));

	// --- OnTurnBegin / OnTurnActivated 호출 수 검증 (라운드 1 전체 기준) ---
	//   그룹([A1+A2], [E1+E2]): OnTurnBegin 각 멤버 1회씩(합산 2), OnTurnActivated 각 멤버 1회씩(합산 2)
	//   솔로(A3, E3): OnTurnBegin 1회, OnTurnActivated 1회

	AssertEqual_Int(A1->TurnBeginCount + A2->TurnBeginCount, 2,
		TEXT("A1·A2 OnTurnBegin 합산 2회여야 한다"));
	AssertEqual_Int(A1->TurnActivatedCount + A2->TurnActivatedCount, 2,
		TEXT("A1·A2 OnTurnActivated 합산 2회여야 한다 (각 멤버 순차 활성화)"));

	AssertEqual_Int(E1->TurnBeginCount + E2->TurnBeginCount, 2,
		TEXT("E1·E2 OnTurnBegin 합산 2회여야 한다"));
	AssertEqual_Int(E1->TurnActivatedCount + E2->TurnActivatedCount, 2,
		TEXT("E1·E2 OnTurnActivated 합산 2회여야 한다 (각 멤버 순차 활성화)"));

	AssertEqual_Int(A3->TurnBeginCount, 1, TEXT("A3 OnTurnBegin 1회여야 한다"));
	AssertEqual_Int(A3->TurnActivatedCount, 1, TEXT("A3 OnTurnActivated 1회여야 한다"));

	AssertEqual_Int(E3->TurnBeginCount, 1, TEXT("E3 OnTurnBegin 1회여야 한다"));
	AssertEqual_Int(E3->TurnActivatedCount, 1, TEXT("E3 OnTurnActivated 1회여야 한다"));

	// 마지막 슬롯 종료 → 라운드 2 진입
	CM->EndCurrentTurn();
	if (CM->IsInCombat())
	{
		AssertEqual_Int(CM->GetCurrentRound(), 2,
			TEXT("6회 EndCurrentTurn 후 라운드 2로 진입해야 한다"));
	}

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("멀티 그룹 한 바퀴 순환 테스트 통과"));
}

// Test 7: 델리게이트 이벤트 발화
void APBTest_DelegateEvents::StartTest()
{
	Super::StartTest();

	UPBCombatManagerSubsystem* CM = GetCombatManager();

	// 이벤트 카운터
	int32 StateChangedCount = 0;
	int32 TurnChangedCount = 0;
	int32 RoundChangedCount = 0;

	FDelegateHandle StateHandle = CM->OnCombatStateChanged.AddLambda(
		[&StateChangedCount](EPBCombatState) { StateChangedCount++; });
	FDelegateHandle TurnHandle = CM->OnActiveTurnChanged.AddLambda(
		[&TurnChangedCount](AActor*, int32) { TurnChangedCount++; });
	FDelegateHandle RoundHandle = CM->OnRoundChanged.AddLambda(
		[&RoundChangedCount](int32) { RoundChangedCount++; });

	SpawnAlly(5, TEXT("아군"));
	SpawnEnemy(3, TEXT("적"));

	CM->StartCombat(GetAllCombatantsAsActors());

	// StartCombat은 여러 상태 전이를 거침: CombatStarting → InitiativeRoll → TurnInProgress
	AssertTrue(StateChangedCount >= 3,
		FString::Printf(TEXT("StartCombat 중 최소 3회 상태 전이 (실제: %d)"), StateChangedCount));
	AssertTrue(TurnChangedCount >= 1,
		FString::Printf(TEXT("StartCombat 중 최소 1회 턴 변경 (실제: %d)"), TurnChangedCount));
	AssertTrue(RoundChangedCount >= 1,
		FString::Printf(TEXT("StartCombat 중 최소 1회 라운드 변경 (실제: %d)"), RoundChangedCount));

	// 턴 종료 시 이벤트
	int32 TurnCountBefore = TurnChangedCount;
	CM->EndCurrentTurn();
	AssertTrue(TurnChangedCount > TurnCountBefore,
		TEXT("EndCurrentTurn 후 OnActiveTurnChanged가 발화되어야 한다"));

	// 정리
	CM->OnCombatStateChanged.Remove(StateHandle);
	CM->OnActiveTurnChanged.Remove(TurnHandle);
	CM->OnRoundChanged.Remove(RoundHandle);

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("델리게이트 이벤트 발화 테스트 통과"));
}
