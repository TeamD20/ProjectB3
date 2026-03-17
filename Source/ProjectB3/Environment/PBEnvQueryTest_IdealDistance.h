// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "PBEnvQueryTest_IdealDistance.generated.h"

/**
 * 커스텀 EQS Test: 이상적 교전 거리 기반 스코어링.
 *
 * Clearinghouse의 EQSIdealDistance 값을 런타임에 읽어서,
 * 후보 포인트와 타겟 간 2D 거리가 이상 거리에 가까울수록 높은 점수를 부여한다.
 *
 * Score = 1.0 - Clamp(|Distance - IdealDistance| / NormalizationRange, 0, 1)
 *
 * 근접 캐릭터: IdealDistance 작음 → 타겟 가까이가 고점수
 * 원거리 캐릭터: IdealDistance = MaxRange × 0.85 → 사거리 끝이 고점수
 *
 * IdealDistance <= 0이면 스코어링 스킵 (모든 포인트 동일 점수).
 */
UCLASS()
class PROJECTB3_API UPBEnvQueryTest_IdealDistance : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UPBEnvQueryTest_IdealDistance();

protected:
	/*~ UEnvQueryTest Interface ~*/
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	// 거리 판정 대상 Context (기본: Context_Target)
	UPROPERTY(EditDefaultsOnly, Category = "IdealDistance")
	TSubclassOf<UEnvQueryContext> TargetContext;

	// 점수 정규화 범위 — |실제거리 - 이상거리|가 이 값 이상이면 점수 0
	// 기본 1500cm: 이상 거리에서 1500cm 이상 벗어나면 최저 점수
	UPROPERTY(EditDefaultsOnly, Category = "IdealDistance", meta = (ClampMin = "100.0"))
	float NormalizationRange = 1500.f;
};
