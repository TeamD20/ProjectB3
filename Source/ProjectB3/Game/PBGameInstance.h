// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBGameInstance.generated.h"

class UPBAbilitySetData;
class UPBAbilitySystemRegistry;
class UPBItemDataAsset;
class UPBEquipmentDataAsset;

// 에셋 번들 로드 완료 콜백
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnBundleLoadComplete, const FGameplayTag&, BundleTag);

// 에셋 번들 로드 실패 콜백
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnBundleLoadFailed, const FGameplayTag&, BundleTag, const FString&, Reason);

// 프리로드 완료 콜백
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPreloadComplete, bool, bSuccess);

// 에셋 번들 정의 — BP에서 로딩 대상 소프트 오브젝트 목록을 설정
USTRUCT(BlueprintType)
struct FPBAssetBundle
{
	GENERATED_BODY()

	// 번들에 포함된 소프트 오브젝트 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bundle")
	TArray<TSoftObjectPtr<UObject>> Assets;
};

// 게임 전역 데이터 및 설정을 보관하는 GameInstance.
UCLASS()
class PROJECTB3_API UPBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/*~ UGameInstance Interface ~*/
	virtual void Init() override;

	/*~ UPBGameInstance Interface ~*/

	// 타이틀 맵에서 메인 맵 전환 전 호출. AbilitySetRegistry 및 프리로드 번들을 비동기 로드 후 프리웜 실행.
	UFUNCTION(BlueprintCallable, Category = "Preload")
	void StartPreload(FOnPreloadComplete OnComplete);

	// 소프트 레퍼런스로 지정된 모든 레지스트리를 동기 로드하고 TSet에 보관
	void LoadAllRegistries();

	// === 에셋 반환 헬퍼 ===
	static const UPBAbilitySystemRegistry* GetAbilitySystemRegistry(const UObject* WorldContextObject);
	static const UPBAbilitySetData* GetCommonAbilitySet(const UObject* WorldContextObject);
	static const UPBAbilitySetData* FindAbilitySetByTag(const UObject* WorldContextObject, const FGameplayTag& Tag);

	// === 아이템 데이터 쿼리 ===

	// 특정 타입의 아이템 DA 목록 반환
	static TArray<UPBItemDataAsset*> GetItemsByType(const UObject* WorldContextObject, EPBItemType Type);

	// 특정 슬롯에 장착 가능한 장비 DA 목록 반환
	static TArray<UPBEquipmentDataAsset*> GetAllEquipmentForSlot(const UObject* WorldContextObject, EPBEquipSlot Slot);

	// 번들 태그로 에셋 비동기 로드 시작. 로드 완료 시 프리웜 인터페이스 검사 후 프리웜 실행, OnComplete 콜백 호출.
	UFUNCTION(BlueprintCallable, Category = "AssetBundle")
	void LoadAssetBundle(const FGameplayTag& BundleTag, FOnBundleLoadComplete OnComplete, FOnBundleLoadFailed OnFailed);

private:
	// 번들 비동기 로드 완료 내부 핸들러
	void OnBundleAssetsLoaded(FGameplayTag BundleTag, FOnBundleLoadComplete OnComplete, FOnBundleLoadFailed OnFailed);

	// AbilitySetRegistry 비동기 로드 완료 내부 핸들러
	void OnRegistryLoaded();

	// 프리로드 번들 로드 성공 내부 핸들러
	UFUNCTION()
	void OnPreloadBundleComplete(const FGameplayTag& BundleTag);

	// 프리로드 번들 로드 실패 내부 핸들러
	UFUNCTION()
	void OnPreloadBundleFailed(const FGameplayTag& BundleTag, const FString& Reason);

	// 프리로드 단계별 완료 체크
	void CheckPreloadComplete();

	// AssetManager 기반 아이템 DA 일괄 로드
	void LoadAllItemData();

protected:
	// ==== 소프트 레퍼런스 (BP에서 지정) ====

	// 어빌리티 셋 레지스트리 DA
	UPROPERTY(EditDefaultsOnly, Category = "Data|Registries")
	TSoftObjectPtr<UPBAbilitySystemRegistry> AbilitySetRegistry;

	// 에셋 번들 매핑 (번들 태그 → 로딩 대상 소프트 오브젝트 목록)
	UPROPERTY(EditDefaultsOnly, Category = "Data|Bundles")
	TMap<FGameplayTag, FPBAssetBundle> AssetBundles;

private:
	// 로드된 에셋 보관 (GC 방지용)
	UPROPERTY()
	TSet<TObjectPtr<UObject>> LoadedAssets;

	// 타입별 아이템 DA 인덱스 (GC 루팅은 LoadedAssets가 담당)
	TMap<EPBItemType, TArray<TObjectPtr<UPBItemDataAsset>>> CategoryIndex;

	// 진행 중인 번들 비동기 로드 핸들 (GC 방지 및 취소용)
	TMap<FGameplayTag, TSharedPtr<FStreamableHandle>> PendingBundleHandles;

	// AbilitySetRegistry 비동기 로드 핸들
	TSharedPtr<FStreamableHandle> RegistryLoadHandle;

	// 프리로드 진행 상태
	FOnPreloadComplete PreloadCompleteCallback;

	// 프리로드 중 실패 여부
	bool bPreloadFailed = false;

	// 프리로드 잔여 작업 수 (레지스트리 + 번들)
	int32 PreloadPendingCount = 0;
};
