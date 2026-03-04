// PBGenerateSequenceTask.cpp

#include "PBGenerateSequenceTask.h"
#include "Engine/World.h"
#include "PBUtilityClearinghouse.h"
#include "StateTreeExecutionContext.h"

// StateTree 디버깅을 위한 독립적인 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogPBStateTree, Log, All);

/*~ 상태 진입 실행 로직 ~*/

EStateTreeRunStatus UPBGenerateSequenceTask::EnterState(
    FStateTreeExecutionContext &Context,
    const FStateTreeTransitionResult &Transition) {

  // UCLASS 버전에서는 InstanceData 구조체 없이 멤버 변수를 직접 사용합니다.
  // TODO: 탬플릿 ( 행동조합 ) 을 만들어야함

  // 1. 구동 주체 유효성 검증
  if (!IsValid(SelfActor)) {
    UE_LOG(LogPBStateTree, Error,
           TEXT("GenerateSequenceTask: SelfActor가매핑되지 않았거나 유효하지 "
                "않습니다."));
    return EStateTreeRunStatus::Failed;
  }

  // 2. 월드 생태계를 통한 Clearinghouse 서브시스템 획득
  UWorld *World = SelfActor->GetWorld();
  if (!IsValid(World)) {
    return EStateTreeRunStatus::Failed;
  }

  UPBUtilityClearinghouse *Clearinghouse =
      World->GetSubsystem<UPBUtilityClearinghouse>();
  if (!IsValid(Clearinghouse)) {
    UE_LOG(LogPBStateTree, Error,
           TEXT("GenerateSequenceTask: Clearinghouse 서브시스템을 찾을 수 "
                "없습니다."));
    return EStateTreeRunStatus::Failed;
  }

  // 3. 턴 시작 데이터 캐싱 수행
  Clearinghouse->CacheTurnData(SelfActor);

  // 4. 캐싱된 멀티 타겟 평가 (Multi-Target Evaluation)
  const TArray<TWeakObjectPtr<AActor>> &CachedTargets =
      Clearinghouse->GetCachedTargets();

  AActor *BestTargetActor = nullptr;
  float BestTotalScore = -1.0f;

  UE_LOG(LogPBStateTree, Display,
         TEXT("=== 다중 타겟 평가(Multi-Target Evaluation) 시작 ==="));

  for (TWeakObjectPtr<AActor> WeakTarget : CachedTargets) {
    AActor *PotentialTarget = WeakTarget.Get();
    if (!IsValid(PotentialTarget))
      continue;

    float DistanceScore =
        Clearinghouse->GetNormalizedDistanceToTarget(PotentialTarget);
    float VulnerabilityScore =
        Clearinghouse->GetTargetVulnerabilityScore(PotentialTarget);

    // 퍼지 논리(AND 교집합 연산) 적용: 거리와 체력 조건 중 더 낮은(불리한)
    // 점수를 최종 타겟 매력도로 산정
    float TotalScore = FMath::Min(DistanceScore, VulnerabilityScore);

    UE_LOG(LogPBStateTree, Log,
           TEXT("Target [%s] 평가됨 -> 퍼지 총 점수: %f (Dist: %f, Vuln: %f)"),
           *PotentialTarget->GetName(), TotalScore, DistanceScore,
           VulnerabilityScore);

    if (TotalScore > BestTotalScore) {
      UE_LOG(LogPBStateTree, Log,
             TEXT(">> Target [%s] scored %f -> New Best Target!"),
             *PotentialTarget->GetName(), TotalScore);
      BestTotalScore = TotalScore;
      BestTargetActor = PotentialTarget;
    }
  }

  // 5. 시퀀스 객체 동적 생성
  // (이후 ExecuteSequenceTask로 넘겨줘야 하므로 UObject 메모리 할당)
  UPBActionSequence *NewSequence = NewObject<UPBActionSequence>(SelfActor);

  // 유틸리티 총합 점수 계산
  NewSequence->TotalUtilityScore =
      BestTotalScore > 0.0f ? BestTotalScore : 0.0f;

  // 6. 모의(Mock) 조합 시퀀스 큐잉 (최적의 타겟이 있을 경우만)
  if (IsValid(BestTargetActor)) {
    UE_LOG(LogPBStateTree, Display,
           TEXT("=== 최적의 타겟 [%s]을 대상으로 시퀀스 콤보 생성 개시 ==="),
           *BestTargetActor->GetName());

    // - 가상의 이동 액션 적재
    FPBSequenceAction MoveAction;
    MoveAction.ActionType = EPBActionType::Move;
    MoveAction.TargetActor = BestTargetActor;
    MoveAction.Cost.MovementCost = 300.0f; // 가상의 이동 소모 비용
    NewSequence->EnqueueAction(MoveAction);

    // - 가상의 공격 액션 적재
    FPBSequenceAction AttackAction;
    AttackAction.ActionType = EPBActionType::Attack;
    AttackAction.TargetActor = BestTargetActor;
    AttackAction.Cost.ActionPoints = 1; // 기본 공격 AP 1 소모
    NewSequence->EnqueueAction(AttackAction);
  } else {
    UE_LOG(LogPBStateTree, Warning,
           TEXT("GenerateSequenceTask: 유효한 최고 점수 타겟을 찾지 "
                "못했습니다. 액션을 비웁니다."));
  }

  // 7. 완성된 시퀀스를 Output Data에 바인딩
  GeneratedSequence = NewSequence;

  UE_LOG(LogPBStateTree, Display,
         TEXT("=== GenerateSequenceTask 분석 완료 ==="));
  UE_LOG(LogPBStateTree, Display,
         TEXT("AI [%s]가 생성한 Action Sequence의 TotalUtilityScore: %f"),
         *SelfActor->GetName(), NewSequence->TotalUtilityScore);
  UE_LOG(LogPBStateTree, Display,
         TEXT("결정된 행동 순서 큐: 1. Move -> 2. Attack"));

  // 조립이 성공적으로 끝났으므로 StateTree가 다음 Task(ExecuteSequenceTask
  // 등)로 넘어가도록 Succeeded 반환
  return EStateTreeRunStatus::Running;
}
