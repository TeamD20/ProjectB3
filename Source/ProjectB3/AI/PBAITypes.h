// PBAITypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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

	// 위치 기반 이동 시 사용 (Fallback 등). TargetActor가 null이면 이 좌표를 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	FVector TargetLocation = FVector::ZeroVector;

	// 행동 발생 코스트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	FPBCostData Cost;

	// 실행할 어빌리티의 이벤트 트리거 태그
	// Execute에서 HandleGameplayEvent(AbilityTag, &EventData)로 발동
	// Move의 경우 비어있음 (기존 Ability_Active_Move 경로 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
	FGameplayTag AbilityTag;
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
// Damage_Process.md 연동 공식:
//   ActionScore = (ExpectedDamage × TargetModifier + SituationalBonus) × ArchetypeWeight
// ExpectedDamage는 명중 확률을 내포한 유효 기대 피해량 (GetExpectedXxxDamage 결과)
USTRUCT(BlueprintType)
struct FPBTargetScore
{
	GENERATED_BODY()

	// 평가 대상 액터
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 유효 기대 피해량 (명중 확률 내포)
	// DiceSpec.RollType에 따라:
	//   HitRoll     → GetExpectedHitDamage()
	//   SavingThrow → GetExpectedSavingThrowDamage()
	//   None        → GetExpectedDirectDamage()
	// TODO: Phase 2에서 어빌리티 DiceSpec 기반 실값 연결
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float ExpectedDamage = 0.0f;

	// 이 점수를 산출한 어빌리티의 이벤트 트리거 태그
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	FGameplayTag AbilityTag;

	// 대상 보정 배수 (ThreatMultiplier × RoleMultiplier)
	// TODO: ThreatScore, 역할 시스템 연동 후 실값 교체
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float TargetModifier = 1.0f;

	// 상황 보너스 (환경 상호작용, 처치 보너스 등)
	// TODO: 지형/CC/집중 파괴 등 상황 시스템 연동
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float SituationalBonus = 0.0f;

	// 아키타입 가중치 (행동 카테고리별)
	// TODO: Archetype 데이터 에셋 연동
	UPROPERTY(BlueprintReadWrite, Category = "AI|Scoring")
	float ArchetypeWeight = 1.0f;

	// 최종 ActionScore 산출
	// 공식: (ExpectedDamage × TargetModifier + SituationalBonus) × ArchetypeWeight
	float GetActionScore() const
	{
		return (ExpectedDamage * TargetModifier + SituationalBonus) * ArchetypeWeight;
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
