// PBExecuteSequenceTask.cpp

#include "PBExecuteSequenceTask.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "StateTreeExecutionContext.h"

// StateTree 디버깅을 위한 독립적인 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogPBStateTreeExec, Log, All);

UPBExecuteSequenceTask::UPBExecuteSequenceTask(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  bShouldCallTick = true;
}

/*~ 상태 진입 실행 로직 ~*/

EStateTreeRunStatus UPBExecuteSequenceTask::EnterState(
    FStateTreeExecutionContext &Context,
    const FStateTreeTransitionResult &Transition) {
  // 1. 구동 주체 및 시퀀스 데이터 유효성 검증
  if (!IsValid(SelfActor)) {
    UE_LOG(LogPBStateTreeExec, Error,
           TEXT("ExecuteSequenceTask: SelfActor가 유효하지 않습니다."));
    return EStateTreeRunStatus::Failed;
  }

  if (SequenceToExecute.IsEmpty()) {
    UE_LOG(LogPBStateTreeExec, Warning,
           TEXT("ExecuteSequenceTask: 전달받은 Sequence 데이터 구조체가 "
                "비어있어 실행을 즉시 종료합니다."));
    return EStateTreeRunStatus::Succeeded;
  }

  APawn *PawnSelf = Cast<APawn>(SelfActor);
  if (PawnSelf) {
    CachedAIController = Cast<AAIController>(PawnSelf->GetController());
  } else {
    UE_LOG(
        LogPBStateTreeExec, Display,
        TEXT("ExecuteTask: Pawn이 아니므로 Controller를 캐싱할 수 없습니다."));
  }

  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("\n============================================="));
  UE_LOG(LogPBStateTreeExec, Display, TEXT("AI [%s] 행동 시퀀스 실행 시작!"),
         *SelfActor->GetName());
  UE_LOG(LogPBStateTreeExec, Display, TEXT("기대 총합 유틸리티 점수: %f"),
         SequenceToExecute.TotalUtilityScore);
  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("============================================="));

  bIsActionInProgress = false;
  CurrentAction = SequenceToExecute.SingleAction;

  return ProcessSingleAction();
}

void UPBExecuteSequenceTask::ExitState(
    FStateTreeExecutionContext &Context,
    const FStateTreeTransitionResult &Transition) {
  if (CachedAIController && bIsActionInProgress &&
      CurrentAction.ActionType == EPBActionType::Move) {
    UE_LOG(LogPBStateTreeExec, Display,
           TEXT("ExitState: Cancelling active movement."));
    CachedAIController->StopMovement();
  }
}

EStateTreeRunStatus UPBExecuteSequenceTask::ProcessSingleAction() {
  if (CurrentAction.ActionType == EPBActionType::None) {
    UE_LOG(LogPBStateTreeExec, Warning,
           TEXT("실행 가능한 행동이 없습니다. 실행기 종료."));
    return EStateTreeRunStatus::Succeeded;
  }

  bIsActionInProgress = true;
  FString TargetName = IsValid(CurrentAction.TargetActor)
                           ? CurrentAction.TargetActor->GetName()
                           : TEXT("None");

  switch (CurrentAction.ActionType) {
  case EPBActionType::Move: {
    UE_LOG(LogPBStateTreeExec, Display, TEXT("Executing MOVE: Target [%s]"),
           *TargetName);

    // 이동 소모력(Movement) 차감
    UAbilitySystemComponent *ASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);
    if (IsValid(ASC)) {
      ASC->ApplyModToAttributeUnsafe(
          UPBTurnResourceAttributeSet::GetMovementAttribute(),
          EGameplayModOp::Additive, -CurrentAction.Cost.MovementCost);
    }

    if (CachedAIController && IsValid(CurrentAction.TargetActor)) {
      FAIMoveRequest MoveReq;
      MoveReq.SetGoalActor(CurrentAction.TargetActor);
      MoveReq.SetAcceptanceRadius(50.f);

      FPathFollowingRequestResult Result = CachedAIController->MoveTo(MoveReq);

      if (Result.Code == EPathFollowingRequestResult::Failed) {
        bIsActionInProgress = false;
        return EStateTreeRunStatus::Failed; // 이동경로 탐색 실패 시 명시적 실패
                                            // 반환
      }
    } else {
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed; // 유효하지 않은 이동 명령 시 실패
    }
    break;
  }
  case EPBActionType::Attack: {
    UE_LOG(LogPBStateTreeExec, Display, TEXT("Executing Attack on [%s]."),
           *TargetName);

    // 실제 Action(AP) 차감 로직
    UAbilitySystemComponent *ASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);
    if (IsValid(ASC)) {
      // 비용만큼 Action 스탯 즉시 차감 (임시 하드코딩 된 어트리뷰트 직접 수정)
      // *주의: 실제 프로덕션에서는 Gameplay Effect를 통해 차감해야 리플리케이션
      // 및 버프/디버프 연산이 안전합니다.*
      ASC->ApplyModToAttributeUnsafe(
          UPBTurnResourceAttributeSet::GetActionAttribute(),
          EGameplayModOp::Additive, -CurrentAction.Cost.ActionCost);

      UE_LOG(LogPBStateTreeExec, Display,
             TEXT("Attack 실행 완료: AP %f 소모됨."),
             CurrentAction.Cost.ActionCost);
    }

    bIsActionInProgress = false;
    return EStateTreeRunStatus::Succeeded;
  }
  default:
    bIsActionInProgress = false;
    return EStateTreeRunStatus::Succeeded;
  }

  return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus
UPBExecuteSequenceTask::Tick(FStateTreeExecutionContext &Context,
                             const float DeltaTime) {
  if (!bIsActionInProgress) {
    return EStateTreeRunStatus::Succeeded;
  }
  return UpdateCurrentAction(DeltaTime);
}

EStateTreeRunStatus
UPBExecuteSequenceTask::UpdateCurrentAction(float DeltaTime) {
  if (CurrentAction.ActionType == EPBActionType::Move) {
    if (CachedAIController) {
      EPathFollowingStatus::Type MoveStatus =
          CachedAIController->GetMoveStatus();
      if (MoveStatus == EPathFollowingStatus::Idle) {
        // 이동 완료 또는 실패 (Idle 상태로 떨어짐)
        bIsActionInProgress = false;
      }
    } else {
      bIsActionInProgress = false;
    }
  }

  return EStateTreeRunStatus::Running;
}
