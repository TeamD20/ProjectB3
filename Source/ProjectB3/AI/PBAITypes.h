// PBAITypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "PBAITypes.generated.h"


// 단일 행동의 종류를 정의하는 열거형
// AI 시퀀스의 각 행동이 "무엇을 하는가"를 분류
UENUM(BlueprintType)
enum class EPBActionType : uint8
{
	None,
	Move,       // 위치 이동
	Attack,     // 적 대상 데미지
	Heal,       // 아군 대상 회복
	Buff,       // 아군 대상 강화
	Debuff,     // 적 대상 약화
	Control,    // 적 대상 행동 제한 (CC)
	UseItem     // 아이템 사용 (향후 확장)
};

// 전투 역할 (AI 스코어링의 RoleMultiplier 산출용)
// Character.Class.* 태그로부터 매핑
UENUM(BlueprintType)
enum class EPBCombatRole : uint8
{
	Melee,    // 근접 — Fighter 등
	Ranged,   // 원거리 — Ranger 등
	Caster,   // 마법 — Magician 등
	Healer,   // 회복 (향후 확장)
	Tank      // 탱커 (향후 확장)
};

// 행동에 필요한 코스트 데이터
USTRUCT(BlueprintType)
struct FPBCostData
{
	GENERATED_BODY()

	// 소모할 행동
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	float ActionCost = 0.0f;

	// 소모할 보조 행동
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	float BonusActionCost = 0.0f;

	// 소모할 이동 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	float MovementCost = 0.0f;
};

// 정규화된(0.0 ~ 1.0) 수치 모음 컨텍스트
USTRUCT(BlueprintType)
struct FPBCombatContext
{
	GENERATED_BODY()

	// 타겟과의 거리 적합도 (거리가 알맞을수록 1.0)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Utility")
	float NormalizedDistance = 0.0f;

	// 타겟의 취약성 점수 (HP & 명중률 계산 )
	UPROPERTY(BlueprintReadWrite, Category = "AI|Utility")
	float TargetVulnerability = 0.0f;

	// 고지대 이점 수치 (Z 높이 비교 및 Block 확인)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Utility")
	float HighGroundAdvantage = 0.0f;
};

// 큐에 들어갈 단일 행동 객체
USTRUCT(BlueprintType)
struct FPBSequenceAction
{
	GENERATED_BODY()

	// 어떤 행동을 할 것인가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	EPBActionType ActionType = EPBActionType::None;

	// 이 행동의 타겟 (해당 액터를 목표로 이동, 공격 등 수행)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 위치 기반 이동 시 사용 (Fallback 등). TargetActor가 null이면 이 좌표를 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	FVector TargetLocation = FVector::ZeroVector;

	// 행동 발생 코스트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	FPBCostData Cost;

	// 실행할 어빌리티의 이벤트 트리거 태그 (로깅/디버그용으로 유지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	FGameplayTag AbilityTag;

	// 실행할 어빌리티의 Spec 핸들 (InternalTryActivateAbility용)
	// HandleGameplayEvent 대신 직접 활성화하므로 Blueprint Triggers 설정 불필요
	UPROPERTY(BlueprintReadWrite, Category = "AI|Sequence")
	FGameplayAbilitySpecHandle AbilitySpecHandle;

	// DFS 스코어링에서 사용할 행동 점수 (GetCandidateActions에서 캐싱)
	// AP/BA 혼합 시퀀스에서 어빌리티별 개별 점수를 보존
	float CachedActionScore = 0.0f;

	// ★ Phase 2a: 순수 기대 데미지 (VirtualHP 차감용)
	// CachedActionScore는 (ExpDmg × TargetMod + Bonus) × Archetype이라 HP 차감에 부적합
	float CachedExpectedDamage = 0.0f;

	// MultiTarget 분배 시 사용할 타겟 목록 (중복 허용, 예: [A, A, B])
	// 비어있으면 단일 TargetActor 사용 (SingleTarget/AoE 등)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Sequence")
	TArray<TObjectPtr<AActor>> MultiTargetActors;
};

// 밀치기 예상 결과 — DFS에서 Displacement 어빌리티 선택 시 산출
USTRUCT()
struct FPBPredictedDisplacement
{
	GENERATED_BODY()

	TObjectPtr<AActor> Target = nullptr;      // 밀린 타겟
	FVector PredictedLocation = FVector::ZeroVector; // 밀린 후 예상 위치
	float EstimatedDistance = 300.0f;          // 예상 밀림 거리 (기본 300cm ≈ D&D 10ft)
};

// DFS 탐색 시 각 분기에서 잔여 자원 상태를 추적하는 컨텍스트.
// 값 타입(USTRUCT)이므로 DFS 재귀 호출 시 값 복사로 전달되며,
// 백트래킹 시 이전 스택 프레임의 값으로 자동 복원된다.
USTRUCT(BlueprintType)
struct FPBUtilityContext
{
	GENERATED_BODY()

	// 잔여 행동 포인트 (D&D 5e: 턴당 1)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Context")
	float RemainingAP = 1.0f;

	// 잔여 보조 행동 (D&D 5e: 턴당 1)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Context")
	float RemainingBA = 1.0f;

	// 잔여 이동력 (cm 단위, TurnResourceAttributeSet의 Movement에서 초기화)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Context")
	float RemainingMP = 0.0f;

	// 누적 이동 거리 (AccumulatedMP 가지치기용)
	// DFS 탐색 시 타겟 간 유클리드 거리를 누적하여
	// RemainingMP 초과 시 해당 분기를 즉시 가지치기
	UPROPERTY(BlueprintReadWrite, Category = "AI|Context")
	float AccumulatedMP = 0.0f;

	// 마지막 행동 수행 위치 (AccumulatedMP 계산의 기준점)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Context")
	FVector LastActionLocation = FVector::ZeroVector;

	// 이번 시퀀스에서 넉백/밀치기로 밀려난 타겟 목록.
	// DFS 분기 내에서 Displacement 어빌리티 사용 후,
	// 해당 타겟에 대한 근접 공격 후보를 제외하는 데 사용.
	TSet<TObjectPtr<AActor>> DisplacedTargets;

	// ★ Phase 1 시너지: 밀치기 예상 결과 목록
	// DFS에서 Displacement 선택 시 밀림 방향 + 거리로 예상 위치를 산출하고,
	// 예상 위치가 위험 지역(장판)이면 시너지 보너스를 부여한다.
	TArray<FPBPredictedDisplacement> PredictedDisplacements;

	// 특정 타겟의 예상 위치 반환 (밀림 없으면 현재 위치)
	FVector GetPredictedLocation(const AActor* Target) const
	{
		for (const FPBPredictedDisplacement& PD : PredictedDisplacements)
		{
			if (PD.Target == Target)
			{
				return PD.PredictedLocation;
			}
		}
		return IsValid(Target) ? Target->GetActorLocation() : FVector::ZeroVector;
	}

	// ★ Phase 2a: 가상 HP — DFS에서 공격 시 타겟 HP를 예측 차감.
	// 사망 예측(HP ≤ 0) 타겟은 후속 행동 후보에서 제외하여 오버킬 방지.
	// 값 타입이므로 DFS 분기마다 자동 복사/복원.
	TMap<TObjectPtr<AActor>, float> VirtualHP;

	// 타겟이 사망 예측인지 확인 (VirtualHP ≤ 0)
	bool IsPredictedDead(const AActor* Target) const
	{
		if (const float* HP = VirtualHP.Find(const_cast<AActor*>(Target)))
		{
			return *HP <= 0.0f;
		}
		return false;
	}

	// 타겟의 VirtualHP를 차감
	void ApplyVirtualDamage(AActor* Target, float Damage)
	{
		if (float* HP = VirtualHP.Find(Target))
		{
			*HP = FMath::Max(*HP - Damage, 0.0f);
		}
	}

	// Cost만큼 자원을 차감한다. 자원이 부족하면 false를 반환하고 차감하지 않는다.
	// DFS에서 해당 행동 분기 진입 가능 여부를 판정하는 데 사용.
	bool TryConsumeResources(const FPBCostData& Cost)
	{
		if (RemainingAP < Cost.ActionCost
			|| RemainingBA < Cost.BonusActionCost
			|| RemainingMP < AccumulatedMP + Cost.MovementCost)
		{
			return false;
		}
		RemainingAP -= Cost.ActionCost;
		RemainingBA -= Cost.BonusActionCost;
		AccumulatedMP += Cost.MovementCost;
		return true;
	}

	// NavMesh 실경로 보정 계수.
	// 유클리드 직선 거리에 이 값을 곱해 NavMesh 경로 길이를 보수적으로 근사.
	// 1.3 = 직선 대비 30% 여유 (실측 기반 — 장애물/우회가 많은 맵에서 1.4~1.5 권장)
	static constexpr float PathEstimateFactor = 1.3f;

	// 현재 위치에서 TargetLocation까지의 이동이 잔여 MP 내에서 가능한지 판정.
	// 유클리드 직선 거리 × PathEstimateFactor로 NavMesh 경로를 보수적 추정.
	// false면 확실히 불가능(안전한 가지치기), true여도 실경로는 더 길 수 있음.
	bool CanReachTarget(const FVector& TargetLocation) const
	{
		const float DistToTarget = FVector::Dist(LastActionLocation, TargetLocation);
		return (AccumulatedMP + DistToTarget * PathEstimateFactor) <= RemainingMP;
	}
};

// 순서가 보장된 다중 행동 시퀀스.
// Generate에서 DFS로 채우고, Execute에서 비동기적으로 순차 소비한다.
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBActionSequence
{
	GENERATED_BODY()

	// 이 시퀀스에 포함된 행동 목록 (순서대로 실행)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	TArray<FPBSequenceAction> Actions;

	// 이 행동 조합의 최종 유틸리티 점수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	float TotalUtilityScore = 0.0f;

	// EQS 좌표 최적화 완료 여부 (Generate에서 세팅, Execute에서 확인)
	// false인 동안 Execute는 시퀀스 실행을 대기한다.
	// EQS 미사용 시 기본값 true로 즉시 실행 가능.
	UPROPERTY(BlueprintReadWrite, Category = "AI|Sequence")
	bool bIsReady = true;

	// 현재 실행 중인 행동 인덱스 (ExecuteTask에서 비동기 순차 소비용)
	// 콜백(OnAbilityEnded 등)이 도착할 때마다 1씩 전진
	UPROPERTY(BlueprintReadWrite, Category = "AI|Sequence")
	int32 CurrentActionIndex = 0;

	// 실행할 행동이 남아있는지 확인
	bool HasNextAction() const
	{
		return CurrentActionIndex < Actions.Num();
	}

	// 현재 행동을 반환하고 인덱스를 전진시킨다.
	// 호출 전 반드시 HasNextAction() 확인 필요.
	const FPBSequenceAction& ConsumeNextAction()
	{
		return Actions[CurrentActionIndex++];
	}

	// 시퀀스가 비어있는지(턴 종료 상황) 확인
	bool IsEmpty() const
	{
		return Actions.Num() == 0;
	}
};

// AoE 어빌리티의 최적 배치 후보 (EvaluateAoEPlacements에서 산출)
// NetScore = Σ(적 전술 가치) - Σ(아군 패널티) - 자기 패널티
USTRUCT()
struct FPBAoECandidate
{
	GENERATED_BODY()

	// AoE 중심 좌표 (적 위치 or 클러스터 센트로이드)
	FVector Center = FVector::ZeroVector;

	// 순수 점수 (적 개별 전술 가치 합산 - 아군 패널티 - 자기 패널티)
	float NetScore = 0.0f;

	// 어빌리티 정보
	FGameplayAbilitySpecHandle AbilitySpecHandle;
	FGameplayTag AbilityTag;
	float AoERadius = 0.0f;
	float CastRange = 0.0f;

	// 비용 정보
	FPBCostData Cost;

	// 주타겟 (사망 검증용 — AoE 범위 내 가장 높은 개별 점수를 받은 적)
	TObjectPtr<AActor> PrimaryTarget = nullptr;

	// 행동 유형 (Attack, Debuff, Control)
	EPBActionType ActionType = EPBActionType::Attack;
};

// MultiTarget 어빌리티의 최적 발사체 분배 후보 (EvaluateMultiTargetPlacements에서 산출)
// 전수 열거(Stars & Bars) + KillBonus/OverhealPenalty 적용
USTRUCT()
struct FPBMultiTargetCandidate
{
	GENERATED_BODY()

	// 타겟별 발사체 분배 (중복 허용, 예: [A, A, B] = A에 2발, B에 1발)
	// ExecuteSequenceTask에서 그대로 Payload.TargetActors로 전달
	TArray<TObjectPtr<AActor>> TargetDistribution;

	// 순수 점수 (KillBonus/OverhealPenalty 반영, 전술 배수 적용)
	float NetScore = 0.0f;

	// 어빌리티 정보
	FGameplayAbilitySpecHandle AbilitySpecHandle;
	FGameplayTag AbilityTag;

	// 비용 정보
	FPBCostData Cost;

	// 행동 유형 (Attack, Debuff, Control)
	EPBActionType ActionType = EPBActionType::Attack;
};

// 타겟 1명에 대한 ActionScore 평가 결과
// ActionScore = (ExpectedDamage × TargetModifier + SituationalBonus) × ArchetypeWeight
USTRUCT(BlueprintType)
struct FPBTargetScore
{
	GENERATED_BODY()

	// 평가 대상 액터
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 유효 기대 피해량 (명중 확률 내포, DiceSpec 기반)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float ExpectedDamage = 0.0f;

	// 이 점수를 산출한 어빌리티의 이벤트 트리거 태그 (로깅/디버그용)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	FGameplayTag AbilityTag;

	// 이 점수를 산출한 어빌리티의 Spec 핸들
	// Scoring → Candidate → Execute 파이프라인으로 전달
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	FGameplayAbilitySpecHandle AbilitySpecHandle;

	// 대상 보정 배수 (ThreatMultiplier × RoleMultiplier)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float TargetModifier = 1.0f;

	// 상황 보너스 (FinishOffBonus 등)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float SituationalBonus = 0.0f;

	// 아키타입 가중치 (DataAsset 카테고리별)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float ArchetypeWeight = 1.0f;

	// 최종 ActionScore 산출
	// 공식: (ExpectedDamage × TargetModifier + SituationalBonus) × ArchetypeWeight
	float GetActionScore() const
	{
		return (ExpectedDamage * TargetModifier + SituationalBonus) * ArchetypeWeight;
	}

	// 이동 효율성 점수 (0.0 ~ 1.0, 가까울수록 높음)
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float MovementScore = 1.0f;

	// MovementScore 최종 가중치 (ActionScore와 균형 조절용)
	// 기본 0.5f → ActionScore:MovementScore = 2:1 비율
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float MovementWeight = 0.5f;

	// 최종 통합 점수 (타겟 선정 기준값)
	// TotalScore = ActionScore + MovementScore × MovementWeight
	float GetTotalScore() const
	{
		return GetActionScore() + MovementScore * MovementWeight;
	}
};
