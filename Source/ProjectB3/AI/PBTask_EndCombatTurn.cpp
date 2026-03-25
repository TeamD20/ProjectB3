// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTask_EndCombatTurn.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "Engine/World.h"
#include "StateTreeExecutionContext.h"

// StateTree 디버깅을 위한 독립적인 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogPBStateTreeEndTurn, Log, All);

EStateTreeRunStatus UPBTask_EndCombatTurn::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition)
{
	if (!IsValid(SelfActor))
	{
		UE_LOG(LogPBStateTreeEndTurn, Error,
		       TEXT("EndCombatTurnTask: 일치하는 대상 액터가 없습니다."));
		return EStateTreeRunStatus::Failed;
	}

	UWorld* World = SelfActor->GetWorld();
	if (!IsValid(World))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 전투 매니저 서브시스템 획득
	UPBCombatManagerSubsystem* CombatManager =
		World->GetSubsystem<UPBCombatManagerSubsystem>();
	if (!IsValid(CombatManager))
	{
		UE_LOG(
			LogPBStateTreeEndTurn, Error,
			TEXT(
				"EndCombatTurnTask: PBCombatManagerSubsystem을 찾을 수 없습니다."));
		return EStateTreeRunStatus::Failed;
	}

	// 현재 전투 중인지, 그리고 현재 턴 소유자가 자신인지 확인
	if (!CombatManager->IsInCombat())
	{
		UE_LOG(LogPBStateTreeEndTurn, Warning,
		       TEXT("EndCombatTurnTask: 현재 전투 상태가 아닙니다. 턴 종료 요청을 "
			       "무시합니다."));
		return EStateTreeRunStatus::Failed;
	}

	AActor* CurrentCombatant = CombatManager->GetCurrentCombatant();
	if (CurrentCombatant != SelfActor)
	{
		UE_LOG(LogPBStateTreeEndTurn, Warning,
		       TEXT("EndCombatTurnTask: 현재 턴의 주체가 AI [%s]가 아닙니다. "
			       "(현재 주체: %s)"),
		       *SelfActor->GetName(),
		       CurrentCombatant ? *CurrentCombatant->GetName() : TEXT("None"));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogPBStateTreeEndTurn, Display,
	       TEXT("==== AI [%s]가 더 이상 행동이 불가하여 스스로 턴을 "
		       "종료(EndCurrentTurn)합니다. (%.1f초 후 전환) ===="),
	       *SelfActor->GetName(), TurnTransitionDelay);

	// 딜레이 후 턴 종료 (타이머 기반 비동기)
	// UStateTreeTaskBlueprintBase의 Tick은 ReceiveTick(BP) 구현 여부로 활성화되므로,
	// C++ 전용 Task에서는 월드 타이머를 사용하여 딜레이를 처리한다.
	TWeakObjectPtr<UPBCombatManagerSubsystem> WeakCM = CombatManager;
	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		[WeakCM]()
		{
			if (UPBCombatManagerSubsystem* CM = WeakCM.Get())
			{
				CM->EndCurrentTurn();
			}
		},
		TurnTransitionDelay,
		false);

	// 즉시 Succeeded 반환 → StateTree는 Idle로 복귀.
	// 타이머가 TurnTransitionDelay 후 EndCurrentTurn을 호출하여 다음 턴을 시작한다.
	return EStateTreeRunStatus::Succeeded;
}
