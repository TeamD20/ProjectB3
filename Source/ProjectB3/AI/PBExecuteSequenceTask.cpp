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
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility_Targeted.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/PBGameplayTags.h"
#include "StateTreeExecutionContext.h"
#include "VisualLogger/VisualLogger.h"

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
  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("AI [%s] 행동 시퀀스 실행 시작! (행동 수: %d)"),
         *SelfActor->GetName(), SequenceToExecute.Actions.Num());
  UE_LOG(LogPBStateTreeExec, Display, TEXT("기대 총합 유틸리티 점수: %f"),
         SequenceToExecute.TotalUtilityScore);
  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("============================================="));

  bIsActionInProgress = false;
  bWaitingForSequenceReady = false;
  AbilityTimeoutRemaining = 0.0f;

  // EQS 좌표 최적화 대기: Generate가 아직 EQS 쿼리를 처리 중이면
  // 행동 실행을 보류하고 Tick에서 bIsReady를 폴링한다.
  if (!SequenceToExecute.bIsReady) {
    bWaitingForSequenceReady = true;
    UE_LOG(LogPBStateTreeExec, Display,
           TEXT("ExecuteSequenceTask: EQS 좌표 최적화 대기 중. "
                "Tick에서 준비 완료 후 실행을 시작합니다."));
    return EStateTreeRunStatus::Running;
  }

  // StateTree Input 바인딩은 매 Tick마다 소스(Generate)에서 재복사되어
  // CurrentActionIndex가 0으로 초기화된다. 로컬 복사본으로 실행 상태를 격리한다.
  ExecutionSequence = SequenceToExecute;
  ExecutionSequence.CurrentActionIndex = 0;

  // 첫 번째 행동 시작
  if (!ExecutionSequence.HasNextAction()) {
    return EStateTreeRunStatus::Succeeded;
  }

  CurrentAction = ExecutionSequence.ConsumeNextAction();
  EStateTreeRunStatus Status = ProcessSingleAction();

  // 동기적 완료 (타겟 사망 스킵 등) 시 다음 행동을 즉시 체인
  while (Status == EStateTreeRunStatus::Succeeded &&
         ExecutionSequence.HasNextAction()) {
    CurrentAction = ExecutionSequence.ConsumeNextAction();
    Status = ProcessSingleAction();
  }

  return Status;
}

void UPBExecuteSequenceTask::ExitState(
    FStateTreeExecutionContext &Context,
    const FStateTreeTransitionResult &Transition) {
  bWaitingForSequenceReady = false;

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

EStateTreeRunStatus UPBExecuteSequenceTask::ProcessSingleAction() {
  if (CurrentAction.ActionType == EPBActionType::None) {
    UE_LOG(LogPBStateTreeExec, Warning,
           TEXT("실행 가능한 행동이 없습니다. 실행기 종료."));
    return EStateTreeRunStatus::Succeeded;
  }

  // 타겟 사망 검증: 실행 시점에 타겟이 이미 사망했으면 행동을 스킵
  if (IsValid(CurrentAction.TargetActor)) {
    UAbilitySystemComponent *TargetASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
            CurrentAction.TargetActor);
    if (TargetASC && TargetASC->HasMatchingGameplayTag(
                         PBGameplayTags::Character_State_Dead)) {
      UE_LOG(LogPBStateTreeExec, Warning,
             TEXT("타겟 [%s]이(가) 이미 사망 상태입니다. 행동을 스킵합니다."),
             *CurrentAction.TargetActor->GetName());
      return EStateTreeRunStatus::Succeeded;
    }
  }

  bIsActionInProgress = true;
  FString TargetName = IsValid(CurrentAction.TargetActor)
                           ? CurrentAction.TargetActor->GetName()
                           : TEXT("None");

  // Visual Logger: 행동 실행 시작 기록
  {
    const TCHAR* TypeStr =
      CurrentAction.ActionType == EPBActionType::Attack  ? TEXT("Attack") :
      CurrentAction.ActionType == EPBActionType::Move    ? TEXT("Move") :
      CurrentAction.ActionType == EPBActionType::Heal    ? TEXT("Heal") :
      TEXT("Other");
    UE_VLOG(SelfActor, LogPBStateTreeExec, Log,
      TEXT("[Execute] %s → %s (AP=%.0f, MP=%.0f)"),
      TypeStr, *TargetName,
      CurrentAction.Cost.ActionCost, CurrentAction.Cost.MovementCost);

    // 3D: 행동 대상 방향 세그먼트
    const FVector TargetPos = IsValid(CurrentAction.TargetActor)
      ? CurrentAction.TargetActor->GetActorLocation()
      : CurrentAction.TargetLocation;
    if (!TargetPos.IsZero() && IsValid(SelfActor))
    {
      const FColor SegColor = (CurrentAction.ActionType == EPBActionType::Attack)
        ? FColor::Red : (CurrentAction.ActionType == EPBActionType::Heal)
        ? FColor::Green : FColor::Cyan;
      UE_VLOG_SEGMENT(SelfActor, LogPBStateTreeExec, Log,
        SelfActor->GetActorLocation(), TargetPos, SegColor,
        TEXT("%s→%s"), TypeStr, *TargetName);
    }
  }

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

    // Ability 종료 시 다음 행동으로 전진 (비동기 체인)
    DelegateHandle = CachedASC->OnAbilityEnded.AddLambda(
        [this, bIsFallbackMove](const FAbilityEndedData &AbilityEndedData) {
          if (AbilityEndedData.AbilitySpecHandle == ActiveSpecHandle) {
            AbilityTimeoutRemaining = 0.0f; // 타임아웃 해제
            // Fallback 이동 완료 후 잔여 이동력 전부 소진 (재후퇴 방지)
            if (bIsFallbackMove && CachedASC) {
              CachedASC->ApplyModToAttributeUnsafe(
                  UPBTurnResourceAttributeSet::GetMovementAttribute(),
                  EGameplayModOp::Override, 0.0f);
              UE_LOG(LogPBStateTreeExec, Display,
                     TEXT("[Fallback] 이동 완료 후 이동력 전부 소진."));
            }
            AdvanceToNextAction();
          }
        });

    // 이동 어빌리티 타임아웃 (목표 미도달/EndAbility 미호출 방지)
    AbilityTimeoutRemaining = AbilityTimeoutSeconds;

    // 이동 목표 위치 결정 (EQS 좌표 우선)
    // 1. TargetLocation이 설정됨 (EQS 최적화 또는 Fallback) → 해당 좌표 사용
    // 2. TargetLocation 미설정 + TargetActor 유효 → 액터 위치 fallback
    // 3. 둘 다 없음 → 이동 불가 (ZeroVector)
    const FVector MoveDestination = !CurrentAction.TargetLocation.IsZero()
        ? CurrentAction.TargetLocation
        : (IsValid(CurrentAction.TargetActor)
            ? CurrentAction.TargetActor->GetActorLocation()
            : FVector::ZeroVector);

    UE_LOG(LogPBStateTreeExec, Display,
           TEXT("  이동 목표: (%s) [%s]"),
           *MoveDestination.ToCompactString(),
           !CurrentAction.TargetLocation.IsZero()
               ? TEXT("EQS/설정 좌표")
               : TEXT("TargetActor 위치"));

    // Payload 구성: Location + 이동 목표 좌표
    UPBTargetPayload *MovePayload = NewObject<UPBTargetPayload>(this);
    MovePayload->TargetData.TargetingMode = EPBTargetingMode::Location;
    MovePayload->TargetData.TargetLocations.Add(MoveDestination);

    // InternalTryActivateAbility로 TriggerEventData와 함께 활성화
    // → GA_Move::ActivateAbility에서 AI 경로(HandleMoveEvent)로 직접 진입
    // Attack/Heal과 동일한 패턴. AbilityTriggers와의 충돌 방지.
    FGameplayEventData EventData;
    EventData.EventTag = PBGameplayTags::Event_Movement_MoveCommand;
    EventData.OptionalObject = MovePayload;
    const bool bActivated = CachedASC->InternalTryActivateAbility(
        ActiveSpecHandle, FPredictionKey(), nullptr, nullptr, &EventData);

    if (!bActivated) {
      CachedASC->OnAbilityEnded.Remove(DelegateHandle);
      DelegateHandle.Reset();
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }
    break;
  }
  case EPBActionType::Attack:
  case EPBActionType::Heal:
  case EPBActionType::Buff:
  case EPBActionType::Debuff:
  case EPBActionType::Control: {
    const TCHAR* ActionLabel = [&]() -> const TCHAR* {
        switch (CurrentAction.ActionType)
        {
        case EPBActionType::Attack:  return TEXT("Attack");
        case EPBActionType::Heal:    return TEXT("Heal");
        case EPBActionType::Buff:    return TEXT("Buff");
        case EPBActionType::Debuff:  return TEXT("Debuff");
        case EPBActionType::Control: return TEXT("Control");
        default:                     return TEXT("Unknown");
        }
    }();

    UE_LOG(LogPBStateTreeExec, Display,
           TEXT("Executing %s on [%s]."), ActionLabel, *TargetName);

    // AbilitySpecHandle 유효성 검사 (Scoring 파이프라인에서 전달)
    if (!CurrentAction.AbilitySpecHandle.IsValid()) {
      UE_LOG(LogPBStateTreeExec, Warning,
             TEXT("%s: AbilitySpecHandle이 유효하지 않습니다. (AbilityTag=%s)"),
             ActionLabel, *CurrentAction.AbilityTag.ToString());
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    CachedASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);
    if (!IsValid(CachedASC)) {
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    // SpecHandle로 직접 Spec 조회 (태그 검색 불필요)
    ActiveSpecHandle = CurrentAction.AbilitySpecHandle;

    // 발동 전 자원 기록 (EndAbility가 자원을 실제로 차감했는지 사후 검증용)
    const float ActionCostToConsume = CurrentAction.Cost.ActionCost;
    const float BonusActionCostToConsume = CurrentAction.Cost.BonusActionCost;
    bool bPreAPFound = false;
    const float PreActivationAP = CachedASC->GetGameplayAttributeValue(
        UPBTurnResourceAttributeSet::GetActionAttribute(), bPreAPFound);
    bool bPreBAFound = false;
    const float PreActivationBA = CachedASC->GetGameplayAttributeValue(
        UPBTurnResourceAttributeSet::GetBonusActionAttribute(), bPreBAFound);

    // 어빌리티 종료 시 다음 행동으로 전진 (비동기 체인)
    DelegateHandle = CachedASC->OnAbilityEnded.AddLambda(
        [this, ActionLabel, ActionCostToConsume, BonusActionCostToConsume,
         PreActivationAP, PreActivationBA](
            const FAbilityEndedData &AbilityEndedData) {
          if (AbilityEndedData.AbilitySpecHandle == ActiveSpecHandle) {
            AbilityTimeoutRemaining = 0.0f; // 타임아웃 해제

            UE_LOG(LogPBStateTreeExec, Display,
                   TEXT("%s 어빌리티 종료. (bWasCancelled: %s)"),
                   ActionLabel,
                   AbilityEndedData.bWasCancelled ? TEXT("true")
                                                  : TEXT("false"));

            // 자원 소모 보장: EndAbility의 태그 기반 차감이 정상 동작했는지 검증.
            // AP 검사: ActionCost > 0인 어빌리티가 AP를 차감하지 않았으면 강제 차감.
            if (CachedASC && ActionCostToConsume > 0.0f) {
              bool bPostAPFound = false;
              const float PostAP = CachedASC->GetGameplayAttributeValue(
                  UPBTurnResourceAttributeSet::GetActionAttribute(),
                  bPostAPFound);
              if (bPostAPFound && PostAP >= PreActivationAP) {
                CachedASC->ApplyModToAttributeUnsafe(
                    UPBTurnResourceAttributeSet::GetActionAttribute(),
                    EGameplayModOp::Additive, -ActionCostToConsume);
                UE_LOG(LogPBStateTreeExec, Warning,
                       TEXT("[안전장치] %s — AP 미소모 감지 (Pre=%.0f, "
                            "Post=%.0f), %.0f 강제 차감"),
                       ActionLabel, PreActivationAP, PostAP,
                       ActionCostToConsume);
              }
            }

            // BA 검사: BonusActionCost > 0인 어빌리티가 BA를 차감하지 않았으면 강제 차감.
            if (CachedASC && BonusActionCostToConsume > 0.0f) {
              bool bPostBAFound = false;
              const float PostBA = CachedASC->GetGameplayAttributeValue(
                  UPBTurnResourceAttributeSet::GetBonusActionAttribute(),
                  bPostBAFound);
              if (bPostBAFound && PostBA >= PreActivationBA) {
                CachedASC->ApplyModToAttributeUnsafe(
                    UPBTurnResourceAttributeSet::GetBonusActionAttribute(),
                    EGameplayModOp::Additive, -BonusActionCostToConsume);
                UE_LOG(LogPBStateTreeExec, Warning,
                       TEXT("[안전장치] %s — BA 미소모 감지 (Pre=%.0f, "
                            "Post=%.0f), %.0f 강제 차감"),
                       ActionLabel, PreActivationBA, PostBA,
                       BonusActionCostToConsume);
              }
            }

            AdvanceToNextAction();
          }
        });

    // 어빌리티 실행 타임아웃 시작 (EndMode=Manual에서 EndAbility 미호출 방지)
    AbilityTimeoutRemaining = AbilityTimeoutSeconds;

    // Payload 구성: 어빌리티의 TargetingMode에 따라 분기
    UPBTargetPayload *ActionPayload = NewObject<UPBTargetPayload>(this);

    // AoE 어빌리티 감지: SpecHandle → Spec → CDO의 TargetingMode 확인
    EPBTargetingMode AbilityTargetingMode = EPBTargetingMode::SingleTarget;
    float AbilityAoERadius = 0.f;
    if (const FGameplayAbilitySpec* Spec =
            CachedASC->FindAbilitySpecFromHandle(ActiveSpecHandle))
    {
      if (const UPBGameplayAbility_Targeted* TargetedCDO =
              Cast<UPBGameplayAbility_Targeted>(Spec->Ability))
      {
        AbilityTargetingMode = TargetedCDO->GetTargetingMode();
        AbilityAoERadius = TargetedCDO->GetAoERadius();
      }
    }

    if (AbilityTargetingMode == EPBTargetingMode::AoE)
    {
      // AoE: TargetLocation 우선 (센트로이드 배치), 미설정 시 TargetActor 위치 사용
      const FVector AoECenter = !CurrentAction.TargetLocation.IsZero()
          ? CurrentAction.TargetLocation
          : (IsValid(CurrentAction.TargetActor)
              ? CurrentAction.TargetActor->GetActorLocation()
              : FVector::ZeroVector);
      ActionPayload->TargetData.TargetingMode = EPBTargetingMode::AoE;
      ActionPayload->TargetData.TargetLocations.Add(AoECenter);
      ActionPayload->TargetData.AoERadius = AbilityAoERadius;

      UE_LOG(LogPBStateTreeExec, Display,
             TEXT("  AoE Payload: Center=(%s), Radius=%.0f"),
             *AoECenter.ToCompactString(), AbilityAoERadius);
    }
    else if (AbilityTargetingMode == EPBTargetingMode::MultiTarget
             && !CurrentAction.MultiTargetActors.IsEmpty())
    {
      // MultiTarget: 분배 리스트(중복 허용)를 그대로 Payload에 전달
      ActionPayload->TargetData.TargetingMode = EPBTargetingMode::MultiTarget;
      for (const TObjectPtr<AActor>& Target : CurrentAction.MultiTargetActors)
      {
        if (IsValid(Target))
        {
          ActionPayload->TargetData.TargetActors.Add(
              TWeakObjectPtr<AActor>(Target.Get()));
        }
      }

      UE_LOG(LogPBStateTreeExec, Display,
             TEXT("  MultiTarget Payload: %d명 타겟 (중복 포함)"),
             ActionPayload->TargetData.TargetActors.Num());
    }
    else
    {
      // SingleTarget: 기존 패턴 (타겟 액터 전달)
      ActionPayload->TargetData.TargetingMode = EPBTargetingMode::SingleTarget;
      ActionPayload->TargetData.TargetActors.Add(
          TWeakObjectPtr<AActor>(CurrentAction.TargetActor));
    }

    // InternalTryActivateAbility로 어빌리티 직접 발동
    // HandleGameplayEvent 대신 사용 — Blueprint Triggers 설정 불필요
    // TriggerEventData가 ActivateAbility로 전달되어 AI 경로(Payload 즉시 처리) 활성화
    FGameplayEventData EventData;
    EventData.OptionalObject = ActionPayload;
    const bool bActivated = CachedASC->InternalTryActivateAbility(
        ActiveSpecHandle, FPredictionKey(), nullptr, nullptr, &EventData);

    if (!bActivated) {
      UE_LOG(LogPBStateTreeExec, Warning,
             TEXT("%s: InternalTryActivateAbility 실패. "
                  "SpecHandle=%s, AbilityTag=%s"),
             ActionLabel,
             *ActiveSpecHandle.ToString(),
             *CurrentAction.AbilityTag.ToString());
      CachedASC->OnAbilityEnded.Remove(DelegateHandle);
      DelegateHandle.Reset();
      bIsActionInProgress = false;
      return EStateTreeRunStatus::Failed;
    }

    UE_LOG(LogPBStateTreeExec, Display,
           TEXT("%s 어빌리티 발동 성공. SpecHandle=%s, AbilityTag=[%s]"),
           ActionLabel,
           *ActiveSpecHandle.ToString(),
           *CurrentAction.AbilityTag.ToString());
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
  // Phase 1: EQS 좌표 최적화 완료 대기
  // Generate의 EQS 콜백이 bIsReady를 true로 전환할 때까지 대기.
  // Generate의 Tick 타임아웃 (0.5초)이 안전장치로 동작하므로
  // Execute 측에서는 별도 타임아웃 없이 폴링만 수행한다.
  if (bWaitingForSequenceReady) {
    if (!SequenceToExecute.bIsReady) {
      return EStateTreeRunStatus::Running; // 계속 대기
    }

    // EQS 완료 — 로컬 복사 후 실행 시작
    bWaitingForSequenceReady = false;
    UE_LOG(LogPBStateTreeExec, Display,
           TEXT("ExecuteSequenceTask: EQS 좌표 최적화 완료. "
                "시퀀스 실행을 시작합니다."));

    ExecutionSequence = SequenceToExecute;
    ExecutionSequence.CurrentActionIndex = 0;

    if (!ExecutionSequence.HasNextAction()) {
      return EStateTreeRunStatus::Succeeded;
    }

    CurrentAction = ExecutionSequence.ConsumeNextAction();
    EStateTreeRunStatus Status = ProcessSingleAction();

    // 동기적 완료 시 즉시 체인
    while (Status == EStateTreeRunStatus::Succeeded &&
           ExecutionSequence.HasNextAction()) {
      CurrentAction = ExecutionSequence.ConsumeNextAction();
      Status = ProcessSingleAction();
    }

    return Status;
  }

  // Phase 2: 행동 실행 중 (기존 로직)
  if (!bIsActionInProgress) {
    return EStateTreeRunStatus::Succeeded;
  }
  return UpdateCurrentAction(DeltaTime);
}

EStateTreeRunStatus
UPBExecuteSequenceTask::UpdateCurrentAction(float DeltaTime) {
  // 어빌리티 실행 타임아웃 감시
  // EndMode=Manual 어빌리티에서 Blueprint가 EndAbility를 호출하지 않으면
  // OnAbilityEnded가 영원히 오지 않아 Execute가 무한 대기한다.
  // 타임아웃 시 CancelAbilityHandle → OnAbilityEnded(bWasCancelled=true) 트리거.
  if (AbilityTimeoutRemaining > 0.0f) {
    AbilityTimeoutRemaining -= DeltaTime;
    if (AbilityTimeoutRemaining <= 0.0f) {
      UE_LOG(LogPBStateTreeExec, Warning,
             TEXT("[타임아웃] 어빌리티 %.0f초 내 미종료. 강제 취소합니다."),
             AbilityTimeoutSeconds);
      if (CachedASC && ActiveSpecHandle.IsValid()) {
        CachedASC->CancelAbilityHandle(ActiveSpecHandle);
        // CancelAbilityHandle → OnAbilityEnded 콜백 → AdvanceToNextAction
      } else {
        // ASC 없음 → 직접 다음 행동으로 진행
        AdvanceToNextAction();
      }
    }
  }
  return EStateTreeRunStatus::Running;
}

/*~ 비동기 행동 체인 ~*/

void UPBExecuteSequenceTask::AdvanceToNextAction() {
  // 이전 행동의 델리게이트 정리
  if (CachedASC) {
    CachedASC->OnAbilityEnded.Remove(DelegateHandle);
    DelegateHandle.Reset();
  }

  // 다음 행동 확인
  if (!ExecutionSequence.HasNextAction()) {
    bIsActionInProgress = false;
    UE_LOG(LogPBStateTreeExec, Display,
           TEXT("[시퀀스] 모든 행동 실행 완료. 총 %d개 행동 처리."),
           ExecutionSequence.CurrentActionIndex);

    // Visual Logger: 시퀀스 실행 완료
    UE_VLOG(SelfActor, LogPBStateTreeExec, Log,
      TEXT("[Execute] 시퀀스 완료: %d개 행동 처리"),
      ExecutionSequence.CurrentActionIndex);
    return;
  }

  // 다음 행동 소비 및 실행
  CurrentAction = ExecutionSequence.ConsumeNextAction();
  UE_LOG(LogPBStateTreeExec, Display,
         TEXT("[시퀀스] 다음 행동으로 전진: [%d/%d]"),
         ExecutionSequence.CurrentActionIndex,
         ExecutionSequence.Actions.Num());

  const EStateTreeRunStatus Status = ProcessSingleAction();

  if (Status == EStateTreeRunStatus::Succeeded) {
    // 동기적 완료 (타겟 사망 스킵 등) → 즉시 다음 행동 시도
    AdvanceToNextAction();
  } else if (Status == EStateTreeRunStatus::Failed) {
    // 실행 실패 → 시퀀스 중단
    bIsActionInProgress = false;
    UE_LOG(LogPBStateTreeExec, Warning,
           TEXT("[시퀀스] 행동 실행 실패. 시퀀스를 중단합니다."));
  }
  // Running → 콜백이 다시 AdvanceToNextAction을 호출할 것
}
