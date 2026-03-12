// PBExecuteSequenceTask.cpp

#include "PBExecuteSequenceTask.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/PBGameplayTags.h"
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

  return ProcessSingleAction(Context, Transition);
}

void UPBExecuteSequenceTask::ExitState(
    FStateTreeExecutionContext &Context,
    const FStateTreeTransitionResult &Transition) {
  if (bIsActionInProgress) {
    if (CurrentAction.ActionType == EPBActionType::Move &&
        CachedAIController) {
      UE_LOG(LogPBStateTreeExec, Display,
             TEXT("ExitState: 진행 중인 이동을 취소합니다."));
      CachedAIController->StopMovement();
    }

    // 진행 중인 어빌리티 강제 취소 (Attack 등 GAS 어빌리티)
    if (CachedASC && ActiveSpecHandle.IsValid()) {
      CachedASC->CancelAbilityHandle(ActiveSpecHandle);
      UE_LOG(LogPBStateTreeExec, Display,
             TEXT("ExitState: 진행 중인 어빌리티를 취소합니다."));
    }
  }

  if (CachedASC) {
    CachedASC->OnAbilityEnded.Remove(DelegateHandle);
  }

  DelegateHandle.Reset();
  CachedASC = nullptr;
}

EStateTreeRunStatus UPBExecuteSequenceTask::ProcessSingleAction(
    FStateTreeExecutionContext &Context,
    const FStateTreeTransitionResult &Transition) {
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
    // GAS를 통한 Move Ability 실행
    CachedASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);
    if (!IsValid(CachedASC)) {
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    FGameplayTagContainer TargetTags;
    TargetTags.AddTag(PBGameplayTags::Ability_Active_Move);
    TArray<FGameplayAbilitySpec *> MatchingSpecs;
    CachedASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
        TargetTags, MatchingSpecs);

    if (MatchingSpecs.IsEmpty()) {
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    if (FGameplayAbilitySpec *AbilitySpec = MatchingSpecs[0]) {
      ActiveSpecHandle = AbilitySpec->Handle;
    }

    // Fallback 이동 여부 캡처 (TargetActor가 null이면 Fallback)
    const bool bIsFallbackMove = !IsValid(CurrentAction.TargetActor);

    // Ability 종료 시 bIsActionInProgress를 false로 전환
    DelegateHandle = CachedASC->OnAbilityEnded.AddLambda(
        [this, bIsFallbackMove](const FAbilityEndedData &AbilityEndedData) {
          if (AbilityEndedData.AbilitySpecHandle == ActiveSpecHandle) {
            bIsActionInProgress = false;

            // Fallback 이동 완료 후 잔여 이동력 전부 소진 (재후퇴 방지)
            if (bIsFallbackMove && CachedASC) {
              CachedASC->ApplyModToAttributeUnsafe(
                  UPBTurnResourceAttributeSet::GetMovementAttribute(),
                  EGameplayModOp::Override, 0.0f);
              UE_LOG(LogPBStateTreeExec, Display,
                     TEXT("[Fallback] 이동 완료 후 이동력 전부 소진."));
            }
          }
        });

    const bool bActivated = CachedASC->TryActivateAbility(ActiveSpecHandle);
    if (!bActivated) {
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    // 이동 목표 위치 결정: TargetActor가 있으면 액터 위치, 없으면 TargetLocation
    const FVector MoveDestination = IsValid(CurrentAction.TargetActor)
        ? CurrentAction.TargetActor->GetActorLocation()
        : CurrentAction.TargetLocation;
  
    UPBTargetPayload *MovePayload = NewObject<UPBTargetPayload>(this);
    MovePayload->TargetData.TargetingMode = EPBTargetingMode::Location;
    MovePayload->TargetData.TargetLocations.Add(MoveDestination);
  
    FGameplayEventData EventData;
    EventData.OptionalObject = MovePayload;
    CachedASC->HandleGameplayEvent(PBGameplayTags::Event_Movement_MoveCommand,
                                   &EventData);
    break;
  }
  case EPBActionType::Attack: {
    UE_LOG(LogPBStateTreeExec, Display, TEXT("Executing Attack on [%s]."),
           *TargetName);

    // AbilityTag 유효성 검사
    if (!CurrentAction.AbilityTag.IsValid()) {
      UE_LOG(LogPBStateTreeExec, Warning,
             TEXT("Attack: AbilityTag가 설정되지 않았습니다."));
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    CachedASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);
    if (!IsValid(CachedASC)) {
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    // AbilityTag로 어빌리티 Spec 조회 (OnAbilityEnded 핸들 매칭용)
    FGameplayTagContainer AbilityTags;
    AbilityTags.AddTag(CurrentAction.AbilityTag);
    TArray<FGameplayAbilitySpec *> MatchingSpecs;
    CachedASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
        AbilityTags, MatchingSpecs);

    if (MatchingSpecs.IsEmpty()) {
      UE_LOG(LogPBStateTreeExec, Warning,
             TEXT("Attack: AbilityTag [%s]에 매칭되는 어빌리티를 찾을 수 "
                  "없습니다."),
             *CurrentAction.AbilityTag.ToString());
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    ActiveSpecHandle = MatchingSpecs[0]->Handle;

    // 어빌리티 종료 대기 델리게이트 (Move와 동일 패턴)
    DelegateHandle = CachedASC->OnAbilityEnded.AddLambda(
        [this](const FAbilityEndedData &AbilityEndedData) {
          if (AbilityEndedData.AbilitySpecHandle == ActiveSpecHandle) {
            bIsActionInProgress = false;
            UE_LOG(LogPBStateTreeExec, Display,
                   TEXT("Attack 어빌리티 종료. (bWasCancelled: %s)"),
                   AbilityEndedData.bWasCancelled ? TEXT("true")
                                                  : TEXT("false"));
          }
        });

    // Payload 구성: SingleTarget + 타겟 액터
    UPBTargetPayload *AttackPayload = NewObject<UPBTargetPayload>(this);
    AttackPayload->TargetData.TargetingMode = EPBTargetingMode::SingleTarget;
    AttackPayload->TargetData.TargetActors.Add(
        TWeakObjectPtr<AActor>(CurrentAction.TargetActor));

    // HandleGameplayEvent로 어빌리티 발동 + Payload 전달
    // → PBGameplayAbility_Targeted::ActivateAbility의 TriggerEventData로 수신
    // → CommitAbility가 내부에서 자원(Action) 차감 처리
    FGameplayEventData EventData;
    EventData.OptionalObject = AttackPayload;
    const int32 Triggered = CachedASC->HandleGameplayEvent(
        CurrentAction.AbilityTag, &EventData);

    if (Triggered == 0) {
      UE_LOG(LogPBStateTreeExec, Warning,
             TEXT("Attack: HandleGameplayEvent 트리거 실패. "
                  "AbilityTag [%s]에 등록된 트리거가 없습니다."),
             *CurrentAction.AbilityTag.ToString());
      CachedASC->OnAbilityEnded.Remove(DelegateHandle);
      DelegateHandle.Reset();
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    UE_LOG(LogPBStateTreeExec, Display,
           TEXT("Attack 어빌리티 발동 성공. AbilityTag: [%s], "
                "트리거된 어빌리티 수: %d"),
           *CurrentAction.AbilityTag.ToString(), Triggered);
    break;
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
  // 이동 완료 여부는 OnAbilityEnded 델리게이트가 bIsActionInProgress를
  // false로 전환하므로, Tick에서 별도 폴링 불필요.
  // 향후 타임아웃 등 안전장치 추가 시 이 함수에서 처리.
  return EStateTreeRunStatus::Running;
}
