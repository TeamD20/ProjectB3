// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "PBMMC_DexScore.generated.h"

UCLASS()
class PROJECTB3_API UPBMMC_DexScore : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UPBMMC_DexScore();

	/*~ UGameplayModMagnitudeCalculation Interface ~*/
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
private:
	FGameplayEffectAttributeCaptureDefinition AbilityScoreCaptureDef;
};
