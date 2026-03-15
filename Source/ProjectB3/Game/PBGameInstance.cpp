// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameInstance.h"
#include "Engine/AssetManager.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySystemRegistry.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"
#include "ProjectB3/ItemSystem/Data/PBEquipmentDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBGameInstance, Log, All);


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

	LoadAllItemData();
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

void UPBGameInstance::LoadAllItemData()
{
	CategoryIndex.Reset();

	UAssetManager& AM = UAssetManager::Get();
	TArray<FPrimaryAssetId> AssetIds;
	AM.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("PBItemData")), AssetIds);

	int32 LoadedCount = 0;
	for (const FPrimaryAssetId& Id : AssetIds)
	{
		UObject* Loaded = AM.GetPrimaryAssetObject(Id);
		if (!IsValid(Loaded))
		{
			// 미로드 시 동기 로드
			FSoftObjectPath Path = AM.GetPrimaryAssetPath(Id);
			Loaded = Path.TryLoad();
		}

		if (UPBItemDataAsset* ItemDA = Cast<UPBItemDataAsset>(Loaded))
		{
			LoadedAssets.Add(ItemDA);
			CategoryIndex.FindOrAdd(ItemDA->ItemType).Add(ItemDA);
			++LoadedCount;
		}
	}

	UE_LOG(LogPBGameInstance, Log, TEXT("LoadAllItemData: %d개 아이템 DA 로드 완료"), LoadedCount);
}

TArray<UPBItemDataAsset*> UPBGameInstance::GetItemsByType(const UObject* WorldContextObject, EPBItemType Type)
{
	TArray<UPBItemDataAsset*> Result;

	if (!IsValid(WorldContextObject))
	{
		return Result;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World))
	{
		return Result;
	}

	const UPBGameInstance* GI = Cast<UPBGameInstance>(World->GetGameInstance());
	if (!IsValid(GI))
	{
		return Result;
	}

	if (const TArray<TObjectPtr<UPBItemDataAsset>>* Found = GI->CategoryIndex.Find(Type))
	{
		for (const TObjectPtr<UPBItemDataAsset>& Item : *Found)
		{
			Result.Add(Item.Get());
		}
	}

	return Result;
}

TArray<UPBEquipmentDataAsset*> UPBGameInstance::GetAllEquipmentForSlot(const UObject* WorldContextObject, EPBEquipSlot Slot)
{
	TArray<UPBEquipmentDataAsset*> Result;

	if (!IsValid(WorldContextObject))
	{
		return Result;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World))
	{
		return Result;
	}

	const UPBGameInstance* GI = Cast<UPBGameInstance>(World->GetGameInstance());
	if (!IsValid(GI))
	{
		return Result;
	}

	// Weapon 타입에서 슬롯 필터링
	if (const TArray<TObjectPtr<UPBItemDataAsset>>* Weapons = GI->CategoryIndex.Find(EPBItemType::Weapon))
	{
		for (const TObjectPtr<UPBItemDataAsset>& Item : *Weapons)
		{
			if (UPBEquipmentDataAsset* EquipDA = Cast<UPBEquipmentDataAsset>(Item.Get()))
			{
				if (EquipDA->CanEquipToSlot(Slot))
				{
					Result.Add(EquipDA);
				}
			}
		}
	}

	// Armor 타입에서 슬롯 필터링
	if (const TArray<TObjectPtr<UPBItemDataAsset>>* Armors = GI->CategoryIndex.Find(EPBItemType::Armor))
	{
		for (const TObjectPtr<UPBItemDataAsset>& Item : *Armors)
		{
			if (UPBEquipmentDataAsset* EquipDA = Cast<UPBEquipmentDataAsset>(Item.Get()))
			{
				if (EquipDA->CanEquipToSlot(Slot))
				{
					Result.Add(EquipDA);
				}
			}
		}
	}

	// Trinket 타입에서 슬롯 필터링
	if (const TArray<TObjectPtr<UPBItemDataAsset>>* Trinkets = GI->CategoryIndex.Find(EPBItemType::Trinket))
	{
		for (const TObjectPtr<UPBItemDataAsset>& Item : *Trinkets)
		{
			if (UPBEquipmentDataAsset* EquipDA = Cast<UPBEquipmentDataAsset>(Item.Get()))
			{
				if (EquipDA->CanEquipToSlot(Slot))
				{
					Result.Add(EquipDA);
				}
			}
		}
	}

	return Result;
}
