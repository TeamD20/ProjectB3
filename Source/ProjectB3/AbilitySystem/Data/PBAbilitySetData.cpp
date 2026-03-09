// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySetData.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBAbilitySetData, Log, All);

FPBSourceGrantedHandles UPBAbilitySetData::GrantToAbilitySystem(
	UAbilitySystemComponent* ASC,
	int32 CharacterLevel) const
{
	FPBSourceGrantedHandles Handles;

	if (!IsValid(ASC))
	{
		UE_LOG(LogPBAbilitySetData, Warning, TEXT("[%s] GrantToAbilitySystem: ASC가 유효하지 않습니다."), *GetName());
		return Handles;
	}

	// 어빌리티 부여
	for (const FPBAbilityGrantEntry& Entry : Abilities)
	{
		if (!Entry.IsValidData())
		{
			UE_LOG(LogPBAbilitySetData, Warning, TEXT("[%s] 유효하지 않은 어빌리티 엔트리 건너떀."), *GetName());
			continue;
		}

		if (Entry.RequiredLevel > CharacterLevel)
		{
			continue;
		}

		FGameplayAbilitySpec Spec(Entry.AbilityClass, Entry.AbilityLevel, INDEX_NONE, ASC->GetOwner());
		Spec.GetDynamicSpecSourceTags().AppendTags(Entry.DynamicTags);

		Handles.AbilityHandles.Add(ASC->GiveAbility(Spec));

		UE_LOG(LogPBAbilitySetData, Verbose, TEXT("[%s] 어빌리티 부여: %s (Level: %d)"),
			*GetName(), *Entry.AbilityClass->GetName(), Entry.AbilityLevel);
	}

	// 패시브 GE 부여
	for (const FPBEffectGrantEntry& Entry : PassiveEffects)
	{
		if (!Entry.IsValidData())
		{
			UE_LOG(LogPBAbilitySetData, Warning, TEXT("[%s] 유효하지 않은 GE 엔트리 건너떀."), *GetName());
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(Entry.EffectClass, 1, ASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			Handles.EffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()));

			UE_LOG(LogPBAbilitySetData, Verbose, TEXT("[%s] 패시브 GE 부여: %s"),
				*GetName(), *Entry.EffectClass->GetName());
		}
	}

	UE_LOG(LogPBAbilitySetData, Log, TEXT("[%s] 부여 완료 — 어빌리티 %d개, GE %d개"),
		*GetName(), Handles.AbilityHandles.Num(), Handles.EffectHandles.Num());

	return Handles;
}