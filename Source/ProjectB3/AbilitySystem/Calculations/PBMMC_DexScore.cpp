// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBMMC_DexScore.h"

#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"

UPBMMC_DexScore::UPBMMC_DexScore()
{
	AbilityScoreCaptureDef.AttributeToCapture = UPBCharacterAttributeSet::GetDexterityAttribute();;
	AbilityScoreCaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
	AbilityScoreCaptureDef.bSnapshot = false;
	RelevantAttributesToCapture.Add(AbilityScoreCaptureDef);
}

float UPBMMC_DexScore::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;

	float AbilityScore = 0.f;
	GetCapturedAttributeMagnitude(AbilityScoreCaptureDef, Spec, EvalParams, AbilityScore);

	// 능력치 수정치 공식 사용
	return static_cast<float>(UPBAbilitySystemLibrary::CalcAbilityModifier(AbilityScore));
}
