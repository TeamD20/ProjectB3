// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "PBEnvQueryTest_AbilityRange.generated.h"

/**
 * 커스텀 EQS Test: 어빌리티 사거리 기반 동적 필터.
 *
 * Clearinghouse의 EQSAbilityMaxRange 값을 런타임에 읽어서,
 * 후보 포인트와 타겟 간 2D 거리(XY)가 사거리 이내인지 필터링한다.
 *
 * IsTargetInRange(FVector::DistSquaredXY)와 동일한 2D 거리 기준을 사용하여
 * 어빌리티 시스템과 판정 일관성을 보장한다.
 *
 * MaxRange <= 0이면 사거리 무제한 → 모든 포인트 통과 (필터 스킵).
 */
UCLASS()
class PROJECTB3_API UPBEnvQueryTest_AbilityRange : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UPBEnvQueryTest_AbilityRange();

protected:
	/*~ UEnvQueryTest Interface ~*/
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	// 사거리 판정 대상 Context (기본: Context_Target)
	UPROPERTY(EditDefaultsOnly, Category = "AbilityRange")
	TSubclassOf<UEnvQueryContext> TargetContext;
};
