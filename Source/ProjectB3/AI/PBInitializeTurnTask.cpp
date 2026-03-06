// PBInitializeTurnTask.cpp

#include "PBInitializeTurnTask.h"
#include "Engine/World.h"
#include "PBUtilityClearinghouse.h"
#include "StateTreeExecutionContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBInitTurn, Log, All);

EStateTreeRunStatus UPBInitializeTurnTask::EnterState(
    FStateTreeExecutionContext &Context,
    const FStateTreeTransitionResult &Transition) {

  if (!IsValid(SelfActor)) {
    UE_LOG(LogPBInitTurn, Error,
           TEXT("InitializeTurnTask: SelfActor가 매핑되지 않았습니다."));
    return EStateTreeRunStatus::Failed;
  }

  UWorld *World = SelfActor->GetWorld();
  if (IsValid(World)) {
    if (UPBUtilityClearinghouse *Clearinghouse =
            World->GetSubsystem<UPBUtilityClearinghouse>()) {
      Clearinghouse->RestoreTurnResources(SelfActor);
      UE_LOG(LogPBInitTurn, Display,
             TEXT("=== AI [%s]의 턴 시작. 자원 회복(Start of Turn) ==="),
             *SelfActor->GetName());
    }
  }

  // 이 태스크의 목적(자원 초기화)을 달성했으므로, 하위 상태(Generate 등)가
  // 실행될 수 있도록 부모/자신 State 유지를 위해 Running을 반환합니다.
  return EStateTreeRunStatus::Running;
}
