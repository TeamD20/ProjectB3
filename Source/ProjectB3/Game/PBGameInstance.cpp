// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameInstance.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySystemRegistry.h"

void UPBGameInstance::Init()
{
	Super::Init();
	// TODO: 타이틀 -> 게임 전환시 로드?
	LoadAllRegistries();
}

void UPBGameInstance::LoadAllRegistries()
{
	LoadedAssets.Reset();

	if (!AbilitySetRegistry.IsNull())
	{
		if (UObject* Loaded = AbilitySetRegistry.LoadSynchronous())
		{
			LoadedAssets.Add(Loaded);
		}
	}
}

const UPBAbilitySystemRegistry* UPBGameInstance::GetAbilitySystemRegistry(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	const UPBGameInstance* GI = Cast<UPBGameInstance>(World->GetGameInstance());
	if (!IsValid(GI))
	{
		return nullptr;
	}

	return GI->AbilitySetRegistry.Get();
}

const UPBAbilitySetData* UPBGameInstance::GetCommonAbilitySet(const UObject* WorldContextObject)
{
	if (const UPBAbilitySystemRegistry* AbilityRegistry = GetAbilitySystemRegistry(WorldContextObject))
	{
		return AbilityRegistry->GetCommonAbilitySet();
	}
	return nullptr;
}

const UPBAbilitySetData* UPBGameInstance::FindAbilitySetByTag(const UObject* WorldContextObject, const FGameplayTag& Tag)
{
	if (const UPBAbilitySystemRegistry* AbilityRegistry = GetAbilitySystemRegistry(WorldContextObject))
	{
		return AbilityRegistry->FindAbilitySetByTag(Tag);
	}
	return nullptr;
}
