// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "PBEnvQueryTest_HazardAvoidance.generated.h"

/**
 * 커스텀 EQS Score Test: 위험 영역 회피 점수 산출.
 * EnvironmentSubsystem의 QueryHazardAtPoint를 사용하여
 * DamageArea 영역 내 후보 포인트에 낮은 점수를 부여한다.
 *
 * Score = 위험 영역 밖이면 1.0, 안이면 0.0
 *
 * Attack/Fallback EQS에서 사용하여 장판 위 이동을 기피하도록 유도.
 */
UCLASS()
class PROJECTB3_API UPBEnvQueryTest_HazardAvoidance : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UPBEnvQueryTest_HazardAvoidance();

protected:
	/*~ UEnvQueryTest Interface ~*/
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;
	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
