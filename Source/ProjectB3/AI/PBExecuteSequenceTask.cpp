// PBExecuteSequenceTask.cpp

#include "PBExecuteSequenceTask.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "AbilitySystemGlobals.h"
#include "StateTreeExecutionContext.h"

// StateTree 디버깅을 위한 독립적인 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogPBStateTreeExec, Log, All);

UPBExecuteSequenceTask::UPBExecuteSequenceTask(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  
  
  bShouldCallTick = true;
  bShouldCallTickOnlyOnEvents = false;
}

/*~ 상태 진입 실행 로직 ~*/
EStateTreeRunStatus UPBExecuteSequenceTask::EnterState(
    FStateTreeExecutionContext &Context,
    const FStateTreeTransitionResult &Transition) {

  if (!IsValid(SelfActor)) {
    UE_LOG(LogPBStateTreeExec, Error,
           TEXT("ExecuteSequenceTask: SelfActor가 유효하지 않습니다."));
    return EStateTreeRunStatus::Failed;
  }

  if (!IsValid(SequenceToExecute)) {
    UE_LOG(LogPBStateTreeExec, Error,
           TEXT("ExecuteSequenceTask: 실행할 SequenceToExecute 객체가 "
                "비어있습니다."));
    return EStateTreeRunStatus::Failed;
  }

  if (SequenceToExecute->IsEmpty()) {
    UE_LOG(LogPBStateTreeExec, Warning,
           TEXT("ExecuteSequenceTask: 전달받은 Sequence 큐가 이미 비어있어 "
                "실행을 즉시 종료합니다."));
    return EStateTreeRunStatus::Succeeded;
  }

  APawn *PawnSelf = Cast<APawn>(SelfActor);
  if (PawnSelf) {
    CachedAIController = Cast<AAIController>(PawnSelf->GetController());
  } else {
    UE_LOG(LogPBStateTreeExec, Display, TEXT("Pawn X"));
  }

  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("\n============================================="));
  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("AI [%s] 행동 시퀀스(Combo) 실행 시작!"), *SelfActor->GetName());
  UE_LOG(LogPBStateTreeExec, Display, TEXT("기대 총합 유틸리티 점수: %f"),
         SequenceToExecute->TotalUtilityScore);
  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("============================================="));

  // 초기화
  ActionStep = 1;
  bIsActionInProgress = false;

  // 바로 첫 번째 액션을 프로세스합니다.
  return ProcessNextAction();
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

EStateTreeRunStatus UPBExecuteSequenceTask::ProcessNextAction() {

  if (!CachedAIController) {
    UE_LOG(LogPBStateTreeExec, Display, TEXT("컨트롤러 X"));
  }

  if (SequenceToExecute->DequeueAction(CurrentAction)) {
    bIsActionInProgress = true;
    FString TargetName = IsValid(CurrentAction.TargetActor)
                             ? CurrentAction.TargetActor->GetName()
                             : TEXT("None");

    switch (CurrentAction.ActionType) {
    case EPBActionType::Move: {
      UE_LOG(
          LogPBStateTreeExec, Display,
          TEXT("[Step %d] Executing MOVE: Target [%s], AcceptanceRadius: 50.0"),
          ActionStep, *TargetName);

      if (CachedAIController && IsValid(CurrentAction.TargetActor)) {
        FAIMoveRequest MoveReq;
        MoveReq.SetGoalActor(CurrentAction.TargetActor);
        MoveReq.SetAcceptanceRadius(50.f);
        MoveReq.SetUsePathfinding(true);

        // Debug Requirement 1: Capture and log the MoveTo request result
        FPathFollowingRequestResult Result =
            CachedAIController->MoveTo(MoveReq);

        FString ResultString;
        switch (Result.Code) {
        case EPathFollowingRequestResult::Failed:
          ResultString = TEXT("Failed");
          break;
        case EPathFollowingRequestResult::AlreadyAtGoal:
          ResultString = TEXT("AlreadyAtGoal");
          break;
        case EPathFollowingRequestResult::RequestSuccessful:
          ResultString = TEXT("RequestSuccessful");
          break;
        default:
          ResultString = TEXT("Unknown");
          break;
        }

        UE_LOG(LogPBStateTreeExec, Display,
               TEXT(">> MoveTo Request Result: %s (MoveID: %u)"), *ResultString,
               Result.MoveId.GetID());

        if (Result.Code == EPathFollowingRequestResult::Failed) {
          // 이동 실패라면 현재 액션을 취소하고 다음 틱이나 곧바로 다음 액션을
          // 처리할 수 있도록 False 전환
          bIsActionInProgress = false;
        }
      } else {
        UE_LOG(LogPBStateTreeExec, Warning,
               TEXT("Cannot Move: CachedAIController(%s) or TargetActor(%s) is "
                    "invalid."),
               CachedAIController ? TEXT("Valid") : TEXT("Null"),
               IsValid(CurrentAction.TargetActor) ? TEXT("Valid")
                                                  : TEXT("Null"));
        bIsActionInProgress = false; // 이동 실패 시 바로 다음으로 넘어갑니다.
      }
      break;
    }
    case EPBActionType::Attack: {
      UE_LOG(LogPBStateTreeExec, Display,
             TEXT("[Step %d] Executing: Attacking Target [%s]. Consuming "
                  "AP: %d"),
             ActionStep, *TargetName, CurrentAction.Cost.ActionPoints);

      // UE 5.6: Initialize ASC access
      UAbilitySystemComponent *ASC =
          UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);
      if (ASC) {
        FGameplayTagContainer AttackTags;
        AttackTags.AddTag(
            FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));

        bool bSuccess = ASC->TryActivateAbilitiesByTag(AttackTags);
        if (bSuccess) {
          UE_LOG(LogPBStateTreeExec, Display,
                 TEXT(">> ASC Attack Activated Successfully."));
        } else {
          UE_LOG(LogPBStateTreeExec, Warning,
                 TEXT(">> ASC Attack Failed to Activate. (Check Tags/AP)"));
        }
      } else {
        UE_LOG(LogPBStateTreeExec, Error,
               TEXT(">> No AbilitySystemComponent found on SelfActor!"));
      }

      bIsActionInProgress = false; // Mock으로 즉시 완료 처리
      break;
    }
    case EPBActionType::UseItem: {
      UE_LOG(LogPBStateTreeExec, Display,
             TEXT("[Step %d] Mock Executing: Using Item on Target [%s]."),
             ActionStep, *TargetName);
      bIsActionInProgress = false; // Mock으로 즉시 완료 처리
      break;
    }
    default: {
      UE_LOG(LogPBStateTreeExec, Warning,
             TEXT("[Step %d] Unknown ActionType encountered in Queue!"),
             ActionStep);
      bIsActionInProgress = false;
      break;
    }
    }

    ActionStep++;
    // 새로운 액션을 큐에서 꺼내 수행 중 혹은 대기 중인 상태이므로 `Running`
    // 반환 (Spam 방지)
    return EStateTreeRunStatus::Running;
  }

  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("=== AI [%s] 행동 시퀀스 대기열 소진 완료 ==="),
         *SelfActor->GetName());
  return EStateTreeRunStatus::Succeeded; // Queue가 비면 Succeeded 반환
}

EStateTreeRunStatus
UPBExecuteSequenceTask::Tick(FStateTreeExecutionContext &Context,
                             const float DeltaTime) {
  if (!bIsActionInProgress) {
    // 이전 액션이 끝났으면 다음 액션으로 넘어갑니다.
    return ProcessNextAction();
  }
  return UpdateCurrentAction(DeltaTime);
}

EStateTreeRunStatus
UPBExecuteSequenceTask::UpdateCurrentAction(float DeltaTime) {
  if (CurrentAction.ActionType == EPBActionType::Move) {
    if (CachedAIController) {
      EPathFollowingStatus::Type MoveStatus =
          CachedAIController->GetMoveStatus();

      // Debug Requirement 2: Current Tick and MoveStatus logging
      FString StatusString;
      switch (MoveStatus) {
      case EPathFollowingStatus::Idle:
        StatusString = TEXT("Idle");
        break;
      case EPathFollowingStatus::Waiting:
        StatusString = TEXT("Waiting");
        break;
      case EPathFollowingStatus::Moving:
        StatusString = TEXT("Moving");
        break;
      default:
        StatusString = TEXT("Unknown");
        break;
      }

      // 너무 많은 로그가 찍히는 것을 방지하거나 추적을 원활히 하기 위해 상세히
      // 로깅
      UE_LOG(LogPBStateTreeExec, Verbose, TEXT("Tick: AI [%s] is currently %s"),
             *SelfActor->GetName(), *StatusString);

      if (MoveStatus == EPathFollowingStatus::Idle) {
        // 이동 완료 또는 실패 (Idle 상태가 됨)
        UE_LOG(LogPBStateTreeExec, Display,
               TEXT(">> Move action completed or aborted inside "
                    "PathFollowingComponent."));
        bIsActionInProgress = false;
      }
    } else {
      bIsActionInProgress = false;
    }
  } else {
    // 다른 액션들은 지금은 즉시 완료되므로 여기까지 올 일이 거의 없지만 보완
    bIsActionInProgress = false;
  }

  // Debug Requirement 3: Prevent spamming by properly returning Running
  return EStateTreeRunStatus::Running;
}
