// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "PBEnvQueryTest_LineOfSight.generated.h"

/**
 * 커스텀 EQS Test: EnvironmentSubsystem의 CheckLineOfSight를 사용하여 시야 판정.
 * UE5 내장 Trace_Filter를 대체하여 모든 시야 판정이 EnvironmentSystem을 경유하도록 일원화한다.
 * Filter 모드로 동작하며, 시야가 없는 후보 포인트를 제거한다.
 */
UCLASS()
class PROJECTB3_API UPBEnvQueryTest_LineOfSight : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UPBEnvQueryTest_LineOfSight();

protected:
	/*~ UEnvQueryTest Interface ~*/
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	// 시야 판정 대상 Context (기본: Context_Target)
	// EditAnywhere: EQS 에셋 인스턴스에 직렬화 보장
	UPROPERTY(EditAnywhere, Category = "LineOfSight")
	TSubclassOf<UEnvQueryContext> TargetContext;

	// true: 모든 타겟에 LoS 필요 (Attack Position — 특정 타겟 공격용)
	// false: 타겟 중 1명이라도 LoS 있으면 통과 (Fallback Position — 적이 보이기만 하면 됨)
	UPROPERTY(EditAnywhere, Category = "LineOfSight")
	bool bRequireAllTargets = true;
};
