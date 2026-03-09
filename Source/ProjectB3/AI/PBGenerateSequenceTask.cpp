// PBGenerateSequenceTask.cpp

#include "PBGenerateSequenceTask.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "PBUtilityClearinghouse.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "StateTreeExecutionContext.h"


// StateTree 디버깅을 위한 독립적인 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogPBStateTree, Log, All);

/*~ 상태 진입 실행 로직 ~*/

EStateTreeRunStatus UPBGenerateSequenceTask::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition)
{
	// 1. 구동 주체 유효성 검증
	if (!IsValid(SelfActor))
	{
		UE_LOG(LogPBStateTree, Error,
		       TEXT("GenerateSequenceTask: SelfActor가 매핑되지 않았거나 유효하지 "
			       "않습니다."));
		return EStateTreeRunStatus::Failed;
	}

	// 2. 월드 생태계를 통한 Clearinghouse 서브시스템 획득
	UWorld* World = SelfActor->GetWorld();
	if (!IsValid(World))
	{
		return EStateTreeRunStatus::Failed;
	}

	UPBUtilityClearinghouse* Clearinghouse =
		World->GetSubsystem<UPBUtilityClearinghouse>();
	if (!IsValid(Clearinghouse))
	{
		UE_LOG(LogPBStateTree, Error,
		       TEXT("GenerateSequenceTask: Clearinghouse 서브시스템을 찾을 수 "
			       "없습니다."));
		return EStateTreeRunStatus::Failed;
	}

	// 3. Clearinghouse 캐시 활용 (CacheTurnData는 InitializeTurnTask에서 처리됨)
	// 여기서는 이미 캐싱된 데이터를 토대로 분석만 수행

	// --- 어빌리티 및 자원 체크 (Phase 1: Filter) ---
	UAbilitySystemComponent* ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfActor);

	if (!IsValid(ASC))
	{
		return EStateTreeRunStatus::Failed;
	}

	// 턴 자원 어트리뷰트셋 가져오기 (없으면 이전 Mock 속성 사용 로직 등 Fallback
	// 필요 없으나 일단 가져옴)
	const UPBTurnResourceAttributeSet* TurnResourceSet =
		ASC->GetSet<UPBTurnResourceAttributeSet>();
	float CurrentAction = 0.0f;
	float CurrentBonusAction = 0.0f;
	float CurrentMovement = 0.0f;

	if (TurnResourceSet)
	{
		CurrentAction = TurnResourceSet->GetAction();
		CurrentBonusAction = TurnResourceSet->GetBonusAction();
		CurrentMovement = TurnResourceSet->GetMovement();
	}

	UE_LOG(LogPBStateTree, Display,
	       TEXT("=== 어빌리티 필터링 (Phase 1) 개시 [잔여 자원 Action: %f, "
		       "BonusAction: %f, Movement: %f] ==="),
	       CurrentAction, CurrentBonusAction, CurrentMovement);
	TArray<UGameplayAbility*> ValidAbilities;
	const TArray<FGameplayAbilitySpec>& ActivatableAbilities =
		ASC->GetActivatableAbilities();

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities)
	{
		if (Spec.Ability != nullptr)
		{
			// CanActivateAbility 기본 검사 (쿨다운, 코스트 등)
			if (Spec.Ability->CanActivateAbility(Spec.Handle,
			                                     ASC->AbilityActorInfo.Get()))
			{
				ValidAbilities.Add(Spec.Ability);
				UE_LOG(LogPBStateTree, Log, TEXT("어빌리티 필터 통과: [%s]"),
				       *Spec.Ability->GetName());
			}
			else
			{
				UE_LOG(LogPBStateTree, Log,
				       TEXT("어빌리티 필터 제외 (조건 미달): [%s]"),
				       *Spec.Ability->GetName());
			}
		}
	}

	// 4. 주변의 적대적 액터 중 ActionScore 기반 최적 타겟 선정
	// (이전: CachedTargets[0] 고정 → Phase 2: GetBestActionScoreTarget 점수 기반
	//  → Phase 3: GetTopKTargets Top-K 필터 + MovementScore 통합 TotalScore 기반)
	const TArray<FPBTargetScore> TopTargets = Clearinghouse->GetTopKTargets(3);

	AActor* BestTargetActor = nullptr;
	if (TopTargets.Num() > 0 && IsValid(TopTargets[0].TargetActor))
	{
		// TopTargets는 TotalScore 내림차순 정렬 → [0]이 최적 타겟
		BestTargetActor = TopTargets[0].TargetActor.Get();
	}

	float DistanceScore = 0.0f;
	float VulnerabilityScore = 0.0f;
	float HighGroundScore = 0.0f;

	if (IsValid(BestTargetActor))
	{
		DistanceScore =
			Clearinghouse->GetNormalizedDistanceToTarget(BestTargetActor);
		VulnerabilityScore =
			Clearinghouse->GetTargetVulnerabilityScore(BestTargetActor);
		HighGroundScore =
			Clearinghouse->EvaluateHighGroundAdvantage(BestTargetActor);
	}
	else
	{
		UE_LOG(LogPBStateTree, Warning,
		       TEXT("GenerateSequenceTask: 적합한 TargetActor를 찾지 못하여 턴을 "
			       "종료(Failed)합니다."));
		// 더 이상 생성할 행동이 없으므로(타겟 부재), 루프 탈출을 위해 Failed 반환
		return EStateTreeRunStatus::Failed;
	}

	// 유틸리티 총합 점수 계산 (AND 퍼지 연산 최소화 방식 혹은 단순 합 등 임시
	// 산출식)
	GeneratedSequence.TotalUtilityScore =
		FMath::Min(DistanceScore, VulnerabilityScore);

	// 6. 상황에 따른 모의(Mock) 단일 행동 결정 로직
	UE_LOG(LogPBStateTree, Display, TEXT("=== 시퀀스 조합(Combo) 분석 개시 ==="));

	FPBSequenceAction DecidedAction;

	// 간단한 거리 기반 분기 로직: 타겟과의 실제 거리가 200.0f 이하면 공격, 멀면
	// 이동
	float RealDistance = SelfActor->GetDistanceTo(BestTargetActor);

	// 공격 사거리 이내일 경우
	if (RealDistance <= 200.0f)
	{
		// 안전 장치: 현재 GAS 세팅에서 Cost 처리가 완벽하지 않을 수 있으므로,
		// 명시적으로 Action 잔량이 1.0 이상인지 교차 검증하여 무한 루프를 막습니다.
		if (ValidAbilities.Num() > 0 && CurrentAction >= 1.0f)
		{
			DecidedAction.ActionType = EPBActionType::Attack;
			DecidedAction.TargetActor = BestTargetActor;
			DecidedAction.Cost.ActionCost = 1.0f; // 공격은 1 Action 소모
			UE_LOG(LogPBStateTree, Display,
			       TEXT("결정된 행동: 타겟이 근접하여 거리가 %f이고 발동 가능한 "
				       "스킬이 있으므로 "
				       "[%s] 등 Attack을 결정합니다."),
			       RealDistance, *ValidAbilities[0]->GetName());
		}
		else
		{
			UE_LOG(LogPBStateTree, Warning,
			       TEXT("GenerateSequenceTask: 타겟에 근접했으나 발동 가능한 "
				       "어빌리티(Action/AP 부족 등)가 "
				       "없어 턴을 종료(Failed)합니다."));
			return EStateTreeRunStatus::Failed;
		}
	}
	// 공격 사거리 밖일 경우 (이동 접근 시도)
	else
	{
		// 실제 이동력(Movement) 계산 속성을 활용
		float MovementCost = RealDistance;
		if (TurnResourceSet && CurrentMovement < MovementCost &&
			CurrentMovement <= 10.0f)
		{
			// 남은 이동력마저 없다면 턴 종료 처리 (Failed)
			UE_LOG(LogPBStateTree, Warning,
			       TEXT("GenerateSequenceTask: 타겟에 접근해야 하나 남은 "
				       "이동력(Movement: %f)이 부족하여 턴을 종료(Failed)합니다."),
			       CurrentMovement);
			return EStateTreeRunStatus::Failed;
		}

		DecidedAction.ActionType = EPBActionType::Move;
		DecidedAction.TargetActor = BestTargetActor;
		DecidedAction.Cost.MovementCost = MovementCost;

		UE_LOG(LogPBStateTree, Display,
		       TEXT("결정된 행동: 타겟이 멀어 거리가 %f이므로 [Move]를 결정합니다. "
			       "(예상 소모 이동력: %f)"),
		       RealDistance, MovementCost);
	}

	// 구조체 자체의 변수 오버라이드로 대체 적재
	GeneratedSequence.SingleAction = DecidedAction;

	UE_LOG(LogPBStateTree, Display,
	       TEXT("=== GenerateSequenceTask 분석 완료 ==="));
	UE_LOG(LogPBStateTree, Display,
	       TEXT("AI [%s]가 생성한 Action Sequence의 TotalUtilityScore: %f"),
	       *SelfActor->GetName(), GeneratedSequence.TotalUtilityScore);

	// 조립이 성공적으로 끝났으므로 StateTree가 하위 State(Execute)를
	// 취소시키지 않고 유지할 수 있도록 Running 반환
	// (만약 부모 Task가 Succeeded를 반환하면, 하위 State가 시작되자마자 강제
	// 종료됩니다!)
	return EStateTreeRunStatus::Running;
}
