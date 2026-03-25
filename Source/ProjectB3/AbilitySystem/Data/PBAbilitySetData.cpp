// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySetData.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBAbilitySetData, Log, All);

FPBAbilityGrantedHandles UPBAbilitySetData::GrantToAbilitySystem(
	UAbilitySystemComponent* ASC,
	int32 CharacterLevel) const
{
	FPBAbilityGrantedHandles Handles;

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

	// GE 부여
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

void UPBAbilitySetData::CollectPrewarmChildren_Implementation(TArray<UObject*>& OutChildren)
{
	// 어빌리티 CDO를 자식으로 반환 → 각 어빌리티의 Collect 함수가 재귀 호출됨
	for (const FPBAbilityGrantEntry& Entry : Abilities)
	{
		if (Entry.AbilityClass)
		{
			OutChildren.Add(Entry.AbilityClass.GetDefaultObject());
		}
	}
}
