// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameInstance.h"
#include "Engine/AssetManager.h"
#include "PBPrewarmInterface.h"
#include "PBPrewarmManagerSubsystem.h"
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

void UPBGameInstance::LoadAssetBundle(const FGameplayTag& BundleTag, FOnBundleLoadComplete OnComplete)
{
	const FPBAssetBundle* Bundle = AssetBundles.Find(BundleTag);
	if (!Bundle)
	{
		UE_LOG(LogPBGameInstance, Warning, TEXT("LoadAssetBundle: 번들 태그 [%s]에 매핑된 번들이 없습니다."), *BundleTag.ToString());
		OnComplete.ExecuteIfBound(BundleTag);
		return;
	}

	// 소프트 오브젝트 경로 수집
	TArray<FSoftObjectPath> PathsToLoad;
	for (const TSoftObjectPtr<UObject>& Soft : Bundle->Assets)
	{
		if (!Soft.IsNull())
		{
			PathsToLoad.Add(Soft.ToSoftObjectPath());
		}
	}

	if (PathsToLoad.IsEmpty())
	{
		UE_LOG(LogPBGameInstance, Log, TEXT("LoadAssetBundle: 번들 [%s] 로드 대상이 없습니다."), *BundleTag.ToString());
		OnComplete.ExecuteIfBound(BundleTag);
		return;
	}

	UE_LOG(LogPBGameInstance, Log, TEXT("LoadAssetBundle: 번들 [%s] 비동기 로드 시작 (%d개 에셋)"),
		*BundleTag.ToString(), PathsToLoad.Num());

	// 비동기 로드 요청
	FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();
	TSharedPtr<FStreamableHandle> Handle = Streamable.RequestAsyncLoad(
		PathsToLoad,
		FStreamableDelegate::CreateUObject(this, &UPBGameInstance::OnBundleAssetsLoaded, BundleTag, OnComplete));

	if (Handle.IsValid())
	{
		PendingBundleHandles.Add(BundleTag, Handle);
	}
}

void UPBGameInstance::OnBundleAssetsLoaded(FGameplayTag BundleTag, FOnBundleLoadComplete OnComplete)
{
	PendingBundleHandles.Remove(BundleTag);

	const FPBAssetBundle* Bundle = AssetBundles.Find(BundleTag);
	if (!Bundle)
	{
		OnComplete.ExecuteIfBound(BundleTag);
		return;
	}

	// 로드된 에셋 보관 및 프리웜 대상 수집
	TArray<UObject*> PrewarmRoots;
	for (const TSoftObjectPtr<UObject>& Soft : Bundle->Assets)
	{
		UObject* Loaded = Soft.Get();
		if (IsValid(Loaded))
		{
			LoadedAssets.Add(Loaded);

			// 프리웜 인터페이스 구현 여부 검사
			if (Loaded->GetClass()->ImplementsInterface(UPBPrewarmInterface::StaticClass()))
			{
				PrewarmRoots.Add(Loaded);
			}
		}
	}

	// 프리웜 실행
	if (PrewarmRoots.Num() > 0)
	{
		if (UWorld* World = GetWorld())
		{
			if (UPBPrewarmManagerSubsystem* PrewarmManager = World->GetSubsystem<UPBPrewarmManagerSubsystem>())
			{
				PrewarmManager->ExecutePrewarm(PrewarmRoots);
			}
		}
	}

	UE_LOG(LogPBGameInstance, Log, TEXT("LoadAssetBundle: 번들 [%s] 로드 완료 (%d개 에셋, %d개 프리웜)"),
		*BundleTag.ToString(), Bundle->Assets.Num(), PrewarmRoots.Num());

	OnComplete.ExecuteIfBound(BundleTag);
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
