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

	// 현재 전투 중인지, 그리고 현재 턴 소유자가 자신인지 확인 후 턴 종료 호출
	if (CombatManager->IsInCombat())
	{
		AActor* CurrentCombatant = CombatManager->GetCurrentCombatant();
		if (CurrentCombatant == SelfActor)
		{
			UE_LOG(LogPBStateTreeEndTurn, Display,
			       TEXT("==== AI [%s]가 더 이상 행동이 불가하여 스스로 턴을 "
				       "종료(EndCurrentTurn)합니다. ===="),
			       *SelfActor->GetName());

			// 자신의 턴을 매니저에게 정식으로 반납
			CombatManager->EndCurrentTurn();

			// 턴 반납에 성공했으므로 Task 완료 (Succeeded 반환 시 State Tree가 다음
			// 상태를 탐색하거나 Root로 돌아감)
			return EStateTreeRunStatus::Succeeded;
		}
		else
		{
			UE_LOG(LogPBStateTreeEndTurn, Warning,
			       TEXT("EndCombatTurnTask: 현재 턴의 주체가 AI [%s]가 아닙니다. "
				       "(현재 주체: %s)"),
			       *SelfActor->GetName(),
			       CurrentCombatant ? *CurrentCombatant->GetName() : TEXT("None"
			       ));
			return EStateTreeRunStatus::Failed;
		}
	}
	else
	{
		UE_LOG(LogPBStateTreeEndTurn, Warning,
		       TEXT("EndCombatTurnTask: 현재 전투 상태가 아닙니다. 턴 종료 요청을 "
			       "무시합니다."));
		return EStateTreeRunStatus::Failed;
	}
}
