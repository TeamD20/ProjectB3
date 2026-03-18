// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "PBEnvQueryTest_CoverScore.generated.h"

/**
 * 커스텀 EQS Score Test: 엄폐 품질 점수 산출.
 * 각 후보 위치에서 적들에 대한 LoS를 검사하여,
 * 적에게 노출되지 않는 비율이 높을수록 높은 점수를 부여한다.
 *
 * Score = (TotalEnemies - VisibleEnemies) / TotalEnemies
 *   0.0 = 모든 적에게 노출
 *   1.0 = 모든 적으로부터 완전 엄폐
 *
 * Fallback 후퇴 EQS에서 사용하여 "숨을 수 있는 위치" 우선 선택.
 */
UCLASS()
class PROJECTB3_API UPBEnvQueryTest_CoverScore : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UPBEnvQueryTest_CoverScore();

protected:
	/*~ UEnvQueryTest Interface ~*/
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

	// 시야 판정 대상 Context (적 그룹)
	UPROPERTY(EditAnywhere, Category = "Cover")
	TSubclassOf<UEnvQueryContext> TargetContext;
};
