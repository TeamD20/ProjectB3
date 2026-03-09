// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTask_Idle.h"
#include "StateTreeExecutionContext.h"

// StateTree 디버깅을 위한 독립적인 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogPBStateTreeIdle, Log, All);

UPBTask_Idle::UPBTask_Idle(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  
  bShouldCallTick = false;
}

EStateTreeRunStatus
UPBTask_Idle::EnterState(FStateTreeExecutionContext &Context,
                         const FStateTreeTransitionResult &Transition) {
  UE_LOG(LogPBStateTreeIdle, Display,
         TEXT("PBTask_Idle: 이벤트 대기(Running) 상태에 진입합니다."));
	
  return EStateTreeRunStatus::Running;
}

void UPBTask_Idle::ExitState(FStateTreeExecutionContext &Context,
                             const FStateTreeTransitionResult &Transition) {
  UE_LOG(LogPBStateTreeIdle, Display,
         TEXT("PBTask_Idle: 이벤트 수신 등으로 대기 상태가 "
              "종료(Interrupted)되었습니다."));
}
