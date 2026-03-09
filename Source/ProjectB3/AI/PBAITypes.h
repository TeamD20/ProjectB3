// PBAITypes.h
#pragma once

#include "CoreMinimal.h"
#include "PBAITypes.generated.h"


// 단일 행동의 종류를 정의하는 열거형
UENUM(BlueprintType)
enum class EPBActionType : uint8 { None, Move, Attack, UseItem };

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

	// 행동 발생 코스트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	FPBCostData Cost;
};

// 조합 점수를 관리하고, 단일 행동(Single Action) 결과를 담는 객체
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBActionSequence
{
	GENERATED_BODY()

	// 결정된 행동이 아무것도 없는지(턴 종료 상황인지) 확인
	bool IsEmpty() const
	{
		return SingleAction.ActionType == EPBActionType::None;
	}

	// 이 턴 행동 조합(Combo)이 갖는 최종 유틸리티 결산 점수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	float TotalUtilityScore = 0.0f;

	/*~ 단일 행동 데이터 제공 ~*/

	// 결정된 단 1개의 행동 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	FPBSequenceAction SingleAction;
};

// 타겟 1명에 대한 ActionScore 평가 결과
// AI_System.md §7.1: ActionScore = BaseScore × HitProbability × ArchetypeWeight
// + TargetModifiers (현재 샌드박스 단계: BaseScore=1.0f,
// TargetModifiers=VulnerabilityWeight, SituationalBonus=0.0f)
USTRUCT(BlueprintType)
struct FPBTargetScore
{
	GENERATED_BODY()

	// 평가 대상 액터
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 명중 확률 (0.05 ~ 0.95 클램프)
	// §7.1: (d20 + 공격보정 - 대상 AC) / 20 → 추후 AC 속성 연동으로 교체
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float HitProbability = 0.65f;

	// 취약성 가중치 (0.0 ~ 1.0)
	// §7.1 TargetModifiers: HP 비율 기반 → 추후 Health AS 연동으로 교체
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float VulnerabilityWeight = 0.8f;

	// 아키타입 공격 가중치 (§4.4 Archetype.ScoreWeights.AttackWeight)
	// 현재 1.0f 고정 → 추후 UCurveFloat 에셋 연동
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float ArchetypeWeight = 1.0f;

	// 최종 ActionScore 산출
	// §7.1: BaseScore(1.0f) × HitProbability × ArchetypeWeight +
	// TargetModifiers(Vulnerability)
	float GetActionScore() const
	{
		return HitProbability * VulnerabilityWeight * ArchetypeWeight;
	}

	// 이동 비용 기반 점수 (0.0 ~ 1.0)
	// Phase 3: 거리가 가까울수록 높음 (=이동 낭비 없음)
	// AI_System.md §7.2 PositionScore 경량화 버전
	// 공식: 1.0 - (DistToTarget / MaxMovementRange), 클램프 [0.0, 1.0]
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
