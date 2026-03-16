// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBInventorySlotData.h"
#include "PBInventoryViewModel.generated.h"

class UPBCharacterPreviewComponent;
class UPBInventoryComponent;
class UPBEquipmentComponent;
class UTextureRenderTarget2D;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnInventorySlotUpdatedSignature, int32 /*SlotIndex*/);
DECLARE_MULTICAST_DELEGATE(FOnInventoryFullRefreshSignature);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotUpdatedSignature, EPBEquipSlot /*Slot*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponSetChangedSignature, int32 /*NewWeaponSet*/);

using namespace PBUIDelegate;

// 파티원 단위 인벤토리/장비 상태를 중개하는 Actor-Bound ViewModel
UCLASS()
class PROJECTB3_API UPBInventoryViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	/*~ UPBViewModelBase Interface ~*/
	// Actor와 LocalPlayer를 바인딩하고 인벤토리/장비 컴포넌트 이벤트를 구독
	virtual void InitializeForActor(AActor* InTargetActor, ULocalPlayer* InLocalPlayer) override;

	// 컴포넌트 이벤트 구독을 해제하고 내부 캐시를 정리
	virtual void Deinitialize() override;

	// 인벤토리 슬롯 수를 반환
	UFUNCTION(BlueprintPure, Category = "UI|Inventory")
	int32 GetInventorySlotCount() const { return InventorySlots.Num(); }

	// 특정 인덱스의 인벤토리 슬롯 데이터를 반환
	bool GetInventorySlotData(int32 SlotIndex, FPBInventorySlotData& OutData) const;

	// 특정 장비 슬롯의 데이터를 반환
	bool GetEquipmentSlotData(EPBEquipSlot Slot, FPBInventorySlotData& OutData) const;

	// 캐릭터 이름을 반환
	UFUNCTION(BlueprintPure, Category = "UI|Inventory")
	FText GetCharacterName() const { return CharacterName; }

	// 현재 활성 무기 세트 번호를 반환
	UFUNCTION(BlueprintPure, Category = "UI|Inventory")
	int32 GetActiveWeaponSet() const { return ActiveWeaponSet; }

	// 캐릭터 프리뷰 렌더 타겟을 반환 (PreviewComponent가 없으면 nullptr)
	UTextureRenderTarget2D* GetCharacterRenderTarget() const;

	// 캐릭터 프리뷰 캡처 활성화 여부를 설정
	void SetPreviewCaptureActive(bool bActive);

public:
	// 캐릭터 이름 변경 이벤트
	FOnTextChangedSignature OnCharacterNameChanged;

	// 인벤토리 단일 슬롯 갱신 이벤트
	FOnInventorySlotUpdatedSignature OnInventorySlotUpdated;

	// 인벤토리 전체 갱신 이벤트
	FOnInventoryFullRefreshSignature OnInventoryFullRefresh;

	// 장비 단일 슬롯 갱신 이벤트
	FOnEquipmentSlotUpdatedSignature OnEquipmentSlotUpdated;

	// 활성 무기 세트 변경 이벤트
	FOnWeaponSetChangedSignature OnWeaponSetChanged;

protected:
	// 인벤토리 슬롯 스냅샷 배열
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI|Inventory")
	TArray<FPBInventorySlotData> InventorySlots;

	// 장비 슬롯 스냅샷 맵
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI|Inventory")
	TMap<EPBEquipSlot, FPBInventorySlotData> EquipmentSlots;

private:
	// 아이템 인스턴스를 UI 스냅샷 데이터로 변환
	FPBInventorySlotData BuildSlotData(const FPBItemInstance& ItemInstance) const;

	// 아이템 등급에 대응하는 UI 색상을 반환
	FLinearColor GetRarityColor(EPBItemRarity Rarity) const;

	// 인벤토리/장비 캐시를 전체 재구성
	void RebuildAllSlots();

	// 인벤토리 캐시를 전체 재구성
	void RebuildInventorySlots();

	// 장비 캐시를 전체 재구성
	void RebuildEquipmentSlots();

	// 캐릭터 이름을 갱신하고 이벤트를 브로드캐스트
	void UpdateCharacterName(AActor* InActor);

	// 인벤토리 단일 슬롯 변경 이벤트를 처리
	UFUNCTION()
	void HandleInventorySlotChanged(int32 SlotIndex);

	// 인벤토리 전체 변경 이벤트를 처리
	UFUNCTION()
	void HandleInventoryFullRefreshInternal();

	// 장비 슬롯 변경 이벤트를 처리
	UFUNCTION()
	void HandleEquipmentSlotChanged(EPBEquipSlot Slot);

	// 무기 세트 변경 이벤트를 처리
	UFUNCTION()
	void HandleWeaponSetSwitched(int32 NewActiveSet);

private:
	// 캐시된 인벤토리 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPBInventoryComponent> CachedInventory;

	// 캐시된 장비 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPBEquipmentComponent> CachedEquipment;

	// 캐시된 캐릭터 프리뷰 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPBCharacterPreviewComponent> CachedPreviewComponent;

	// 캐시된 캐릭터 이름
	FText CharacterName;

	// 캐시된 활성 무기 세트
	int32 ActiveWeaponSet = 1;
};
