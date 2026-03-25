// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameInstance.h"
#include "Engine/AssetManager.h"
#include "PBPrewarmInterface.h"
#include "PBPrewarmManagerSubsystem.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySystemRegistry.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"
#include "ProjectB3/ItemSystem/Data/PBEquipmentDataAsset.h"
#include "ProjectB3/PBGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBGameInstance, Log, All);


void UPBGameInstance::Init()
{
	Super::Init();
}

void UPBGameInstance::StartPreload(FOnPreloadComplete OnComplete)
{
	PreloadCompleteCallback = OnComplete;
	bPreloadFailed = false;
	PreloadPendingCount = 0;

	LoadedAssets.Reset();
	LoadAllItemData();

	// 1) AbilitySetRegistry 비동기 로드
	if (!AbilitySetRegistry.IsNull())
	{
		++PreloadPendingCount;
		FStreamableManager& Streamable = UAssetManager::Get().GetStreamableManager();
		RegistryLoadHandle = Streamable.RequestAsyncLoad(
			AbilitySetRegistry.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &UPBGameInstance::OnRegistryLoaded));
	}

	// 2) Asset_Bundle_Preload 태그에 해당하는 번들 로드 및 프리웜
	using namespace PBGameplayTags;
	const FPBAssetBundle* PreloadBundle = AssetBundles.Find(Asset_Bundle_Preload);
	if (PreloadBundle && !PreloadBundle->Assets.IsEmpty())
	{
		++PreloadPendingCount;

		FOnBundleLoadComplete BundleSuccess;
		BundleSuccess.BindDynamic(this, &UPBGameInstance::OnPreloadBundleComplete);

		FOnBundleLoadFailed BundleFailed;
		BundleFailed.BindDynamic(this, &UPBGameInstance::OnPreloadBundleFailed);

		LoadAssetBundle(Asset_Bundle_Preload, BundleSuccess, BundleFailed);
	}

	// 로드 대상이 하나도 없으면 즉시 완료
	if (PreloadPendingCount == 0)
	{
		UE_LOG(LogPBGameInstance, Log, TEXT("StartPreload: 프리로드 대상 없음, 즉시 완료"));
		FOnPreloadComplete CallbackCopy = PreloadCompleteCallback;
		PreloadCompleteCallback.Unbind();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
				[CallbackCopy]() mutable
				{
					CallbackCopy.ExecuteIfBound(true);
				}));
		}
		else
		{
			CallbackCopy.ExecuteIfBound(true);
		}
	}
}

void UPBGameInstance::OnRegistryLoaded()
{
	RegistryLoadHandle.Reset();

	UObject* Loaded = AbilitySetRegistry.Get();
	if (IsValid(Loaded))
	{
		LoadedAssets.Add(Loaded);
		
		if (UWorld* World = GetWorld())
		{
			if (UPBPrewarmManagerSubsystem* PrewarmManager = World->GetSubsystem<UPBPrewarmManagerSubsystem>())
			{
				TArray<UObject*> RootObjects = {Loaded};
				PrewarmManager->ExecutePrewarm(RootObjects);
			}
		}
		
		UE_LOG(LogPBGameInstance, Log, TEXT("StartPreload: AbilitySetRegistry 로드 완료"));
	}
	else
	{
		UE_LOG(LogPBGameInstance, Warning, TEXT("StartPreload: AbilitySetRegistry 로드 실패"));
		bPreloadFailed = true;
	}

	CheckPreloadComplete();
}

void UPBGameInstance::OnPreloadBundleComplete(const FGameplayTag& BundleTag)
{
	UE_LOG(LogPBGameInstance, Log, TEXT("StartPreload: 프리로드 번들 [%s] 로드 성공"), *BundleTag.ToString());
	CheckPreloadComplete();
}

void UPBGameInstance::OnPreloadBundleFailed(const FGameplayTag& BundleTag, const FString& Reason)
{
	UE_LOG(LogPBGameInstance, Warning, TEXT("StartPreload: 프리로드 번들 [%s] 로드 실패 — %s"), *BundleTag.ToString(), *Reason);
	bPreloadFailed = true;
	CheckPreloadComplete();
}

void UPBGameInstance::CheckPreloadComplete()
{
	--PreloadPendingCount;
	if (PreloadPendingCount <= 0)
	{
		PreloadPendingCount = 0;
		const bool bSuccess = !bPreloadFailed;
		FOnPreloadComplete CallbackCopy = PreloadCompleteCallback;
		PreloadCompleteCallback.Unbind();

		UE_LOG(LogPBGameInstance, Log, TEXT("StartPreload: 전체 프리로드 완료 (성공=%s)"),
			bSuccess ? TEXT("true") : TEXT("false"));

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,
				[CallbackCopy, bSuccess]() mutable
				{
					CallbackCopy.ExecuteIfBound(bSuccess);
				}));
		}
		else
		{
			CallbackCopy.ExecuteIfBound(bSuccess);
		}
	}
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
	
	if (!GI->AbilitySetRegistry.IsValid())
	{
		return GI->AbilitySetRegistry.LoadSynchronous();
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

void UPBGameInstance::LoadAssetBundle(const FGameplayTag& BundleTag, FOnBundleLoadComplete OnComplete, FOnBundleLoadFailed OnFailed)
{
	const FPBAssetBundle* Bundle = AssetBundles.Find(BundleTag);
	if (!Bundle)
	{
		UE_LOG(LogPBGameInstance, Warning, TEXT("LoadAssetBundle: 번들 태그 [%s]에 매핑된 번들이 없습니다."), *BundleTag.ToString());
		OnFailed.ExecuteIfBound(BundleTag, TEXT("번들 태그에 매핑된 번들이 없습니다."));
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
		FStreamableDelegate::CreateUObject(this, &UPBGameInstance::OnBundleAssetsLoaded, BundleTag, OnComplete, OnFailed));

	if (Handle.IsValid())
	{
		PendingBundleHandles.Add(BundleTag, Handle);
	}
	else
	{
		UE_LOG(LogPBGameInstance, Warning, TEXT("LoadAssetBundle: 번들 [%s] 비동기 로드 핸들 생성 실패"), *BundleTag.ToString());
		OnFailed.ExecuteIfBound(BundleTag, TEXT("비동기 로드 핸들 생성 실패"));
	}
}

void UPBGameInstance::OnBundleAssetsLoaded(FGameplayTag BundleTag, FOnBundleLoadComplete OnComplete, FOnBundleLoadFailed OnFailed)
{
	PendingBundleHandles.Remove(BundleTag);

	const FPBAssetBundle* Bundle = AssetBundles.Find(BundleTag);
	if (!Bundle)
	{
		OnFailed.ExecuteIfBound(BundleTag, TEXT("로드 완료 후 번들 데이터를 찾을 수 없습니다."));
		return;
	}

	// 로드된 에셋 보관 및 프리웜 대상 수집
	int32 FailedCount = 0;
	TArray<UObject*> PrewarmRoots;
	for (const TSoftObjectPtr<UObject>& Soft : Bundle->Assets)
	{
		UObject* Loaded = Soft.Get();
		if (IsValid(Loaded))
		{
			LoadedAssets.Add(Loaded);

			// 프리웜 인터페이스 구현 객체 또는 Niagara/Sound 에셋 자체를 루트로 수집
			if (Loaded->GetClass()->ImplementsInterface(UPBPrewarmInterface::StaticClass())
				|| Cast<UNiagaraSystem>(Loaded)
				|| Cast<USoundBase>(Loaded))
			{
				PrewarmRoots.Add(Loaded);
			}
		}
		else
		{
			++FailedCount;
			UE_LOG(LogPBGameInstance, Warning, TEXT("LoadAssetBundle: 번들 [%s] 에셋 로드 실패 — %s"),
				*BundleTag.ToString(), *Soft.ToSoftObjectPath().ToString());
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

	UE_LOG(LogPBGameInstance, Log, TEXT("LoadAssetBundle: 번들 [%s] 로드 완료 (%d개 에셋, %d개 프리웜, %d개 실패)"),
		*BundleTag.ToString(), Bundle->Assets.Num(), PrewarmRoots.Num(), FailedCount);

	if (FailedCount > 0)
	{
		OnFailed.ExecuteIfBound(BundleTag, FString::Printf(TEXT("%d개 에셋 로드 실패"), FailedCount));
	}
	else
	{
		OnComplete.ExecuteIfBound(BundleTag);
	}
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
