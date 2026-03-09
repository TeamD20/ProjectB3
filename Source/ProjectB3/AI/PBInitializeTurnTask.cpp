// PBInitializeTurnTask.cpp

#include "PBInitializeTurnTask.h"
#include "Engine/World.h"
#include "PBUtilityClearinghouse.h"
#include "StateTreeExecutionContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBInitTurn, Log, All);

EStateTreeRunStatus UPBInitializeTurnTask::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition)
{
	if (!IsValid(SelfActor))
	{
		UE_LOG(LogPBInitTurn, Error,
		       TEXT("InitializeTurnTask: SelfActor가 매핑되지 않았습니다."));
		return EStateTreeRunStatus::Failed;
	}

	UWorld* World = SelfActor->GetWorld();
	if (IsValid(World))
	{
		if (UPBUtilityClearinghouse* Clearinghouse =
			World->GetSubsystem<UPBUtilityClearinghouse>())
		{
			// 1. 자원 복원 (Action, BonusAction, Reaction, Movement 최대치로 초기화)
			Clearinghouse->RestoreTurnResources(SelfActor);

			// 2. 턴 데이터 캐싱 (주변 타겟 탐색 등 무거운 연산 일원화)
			// GenerateSequenceTask가 캐시를 바로 활용할 수 있도록 여기서 선처리
			Clearinghouse->CacheTurnData(SelfActor);

			UE_LOG(LogPBInitTurn, Display,
			       TEXT("=== AI [%s]의 턴 시작. 자원 회복 및 데이터 캐싱 완료 ==="),
			       *SelfActor->GetName());
		}
	}

	// 이 태스크의 목적(자원 초기화)을 달성했으므로, 하위 상태(Generate 등)가
	// 실행될 수 있도록 부모/자신 State 유지를 위해 Running을 반환합니다.
	return EStateTreeRunStatus::Running;
}
