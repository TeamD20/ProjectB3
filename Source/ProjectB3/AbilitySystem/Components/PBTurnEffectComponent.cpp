// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnEffectComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

void UPBTurnEffectComponent::OnGameplayEffectApplied(
	FActiveGameplayEffectsContainer& ActiveGEContainer,
	FGameplayEffectSpec& GESpec,
	FPredictionKey& PredictionKey) const
{
	if (!bApplyOnInitialApplication)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActiveGEContainer.Owner;
	if (!IsValid(ASC))
	{
		return;
	}

	// 최초 적용 시 TurnEffects를 즉시 발동 (원본 Spec의 SetByCaller 등 데이터 유지)
	for (const TSubclassOf<UGameplayEffect>& EffectClass : TurnEffects)
	{
		if (const UGameplayEffect* EffectCDO = EffectClass.GetDefaultObject())
		{
			FGameplayEffectSpec NewSpec;
			NewSpec.InitializeFromLinkedSpec(EffectCDO, GESpec);
			ASC->ApplyGameplayEffectSpecToSelf(NewSpec);
		}
	}
}
