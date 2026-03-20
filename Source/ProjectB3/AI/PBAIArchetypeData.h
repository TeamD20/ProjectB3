// PBAIArchetypeData.h
// AI 아키타입별 행동 가중치를 정의하는 DataAsset.
// Aggressive / Defensive / Support / Disruptor 등 개성을 부여한다.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PBAIArchetypeData.generated.h"

// AI 캐릭터의 전투 성향(아키타입)을 수치화한 DataAsset.
// EvaluateActionScore에서 카테고리별 가중치를 곱하여 아키타입마다 다른 행동을 선택하게 한다.
UCLASS(BlueprintType)
class PROJECTB3_API UPBAIArchetypeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 공격 행동 (무기 공격, 공격 스킬) 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype",
		meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float AttackWeight = 1.0f;

	// 회복 행동 (힐링 스킬) 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype",
		meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float HealWeight = 1.0f;

	// 버프 행동 (아군 강화) 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype",
		meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float BuffWeight = 1.0f;

	// 디버프 행동 (적 약화) 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype",
		meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float DebuffWeight = 1.0f;

	// 제어 행동 (CC, 넉백, 밀치기 등) 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype",
		meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float ControlWeight = 1.0f;

	// 위치 선점 행동 (이동, 전술적 재배치) 가중치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype",
		meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float PositionWeight = 1.0f;

	// EQS 이상적 교전 거리 배율.
	// 어빌리티 사거리에 이 값을 곱하여 AI가 선호하는 교전 거리를 결정한다.
	// 낮을수록 타겟에 가까이 붙고, 높을수록 사거리 끝에서 싸운다.
	// 예: Fighter=0.5 (근접 선호), Ranger=1.0 (최대 사거리), Caster=0.85
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Archetype|Position",
		meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float IdealDistanceMultiplier = 0.85f;
};
