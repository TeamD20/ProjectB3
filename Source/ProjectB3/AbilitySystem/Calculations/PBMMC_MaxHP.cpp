// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBMMC_MaxHP.h"

#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"

UPBMMC_MaxHP::UPBMMC_MaxHP()
{
	ConstitutionCaptureDef.AttributeToCapture = UPBCharacterAttributeSet::GetConstitutionAttribute();
	ConstitutionCaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
	ConstitutionCaptureDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(ConstitutionCaptureDef);
}

float UPBMMC_MaxHP::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;

	float Constitution = 0.f;
	GetCapturedAttributeMagnitude(ConstitutionCaptureDef, Spec, EvalParams, Constitution);

	// 현재 레벨을 가져옴.
	const int32 Level = FMath::Max(1, static_cast<int32>(Spec.GetLevel()));

	return UPBAbilitySystemLibrary::CalcMaxHPBonus(Constitution, Level);
}
