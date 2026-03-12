// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "PBMMC_MaxHP.generated.h"

/**
 * Constitution 기반 MaxHP 보너스 MMC.
 * MaxHP 보너스 = Constitution Modifier × Level
 */
UCLASS()
class PROJECTB3_API UPBMMC_MaxHP : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	/** 캡처할 어트리뷰트 정의 */
	UPBMMC_MaxHP();

	/*~ UGameplayModMagnitudeCalculation Interface ~*/
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	// Constitution 캡처 정의
	FGameplayEffectAttributeCaptureDefinition ConstitutionCaptureDef;
};
