// PBGenerateSequenceTask.cpp

#include "PBGenerateSequenceTask.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "PBAIMockAttributeSet.h"
#include "PBAIMockCharacter.h"
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

    APBAIMockCharacter *MockSelf = Cast<APBAIMockCharacter>(SelfActor);
    if (MockSelf && MockSelf->GetAttributeSet()) {
      float CurrentAction = MockSelf->GetAttributeSet()->GetAction();
      float CurrentMovement = MockSelf->GetAttributeSet()->GetMovement();

      UE_LOG(LogPBStateTree, Log,
             TEXT("현재 가용 자원 - Action: %f, Movement: %f"), CurrentAction,
             CurrentMovement);

      // - 가상의 이동 액션 적재 (이동력이 충분할 때만)
      float EstimatedDistance = SelfActor->GetDistanceTo(BestTargetActor);
      if (CurrentMovement >= EstimatedDistance &&
          EstimatedDistance > 150.0f) { // 150.0f는 임의의 공격 사거리
        FPBSequenceAction MoveAction;
        MoveAction.ActionType = EPBActionType::Move;
        MoveAction.TargetActor = BestTargetActor;
        MoveAction.Cost.MovementCost = EstimatedDistance;
        NewSequence->EnqueueAction(MoveAction);
        CurrentMovement -= EstimatedDistance;
        UE_LOG(LogPBStateTree, Log,
               TEXT("-> 이동 액션 추가됨 (예상 이동 거리: %f)"),
               EstimatedDistance);
      } else if (EstimatedDistance <= 150.0f) {
        UE_LOG(LogPBStateTree, Log,
               TEXT("-> 대상이 이미 사거리(150) 내에 존재하여 이동 생략"));
      } else {
        UE_LOG(LogPBStateTree, Warning,
               TEXT("-> 이동력 부족! 필요: %f, 현재: %f"), EstimatedDistance,
               CurrentMovement);
      }

      // - 가상의 공격 액션 적재 (액션이 충분할 때만)
      if (CurrentAction >= 1.0f) {
        FPBSequenceAction AttackAction;
        AttackAction.ActionType = EPBActionType::Attack;
        AttackAction.TargetActor = BestTargetActor;
        AttackAction.Cost.ActionCost = 1.0f;
        NewSequence->EnqueueAction(AttackAction);
        UE_LOG(LogPBStateTree, Log,
               TEXT("-> 공격 액션 추가됨 (Cost: 1 Action)"));
      } else {
        UE_LOG(LogPBStateTree, Warning,
               TEXT("-> 액션(Action) 부족! 공격 실패"));
      }
    } else {
      UE_LOG(LogPBStateTree, Error,
             TEXT("SelfActor를 PBAIMockCharacter로 캐스팅할 수 없거나 "
                  "AttributeSet이 없습니다."));
    }
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
