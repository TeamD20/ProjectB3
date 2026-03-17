// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "ProjectB3/UI/Inventory/PBInventorySlotData.h"
#include "PBLootViewModel.generated.h"

class UPBInventoryComponent;
class UPBEquipmentComponent;

DECLARE_MULTICAST_DELEGATE(FOnLootSlotsRefreshed);

/** 루팅 슬롯 1개의 UI 표시용 데이터. 장비/인벤토리 아이템을 단일 배열로 통합 관리 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBLootSlotData
{
	GENERATED_BODY()

	// 아이콘, 수량, 희귀도 등 표시용 스냅샷
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	FPBInventorySlotData SlotData;

	// 대상이 현재 장착 중인 장비인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	bool bIsEquipped = false;

	// 장착 슬롯 (bIsEquipped == true일 때만 유효)
	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	EPBEquipSlot EquipSlot = EPBEquipSlot::MAX;
};

/**
 * 루팅 대상의 인벤토리/장비 데이터를 UI에 제공하는 Actor-Bound ViewModel.
 * 장착 중인 장비 → 인벤토리 아이템 순으로 단일 배열에 병합하여 제공한다.
 */
UCLASS()
class PROJECTB3_API UPBLootViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	/*~ UPBViewModelBase Interface ~*/
	/** 루팅 대상 액터와 LocalPlayer를 바인딩하고 컴포넌트 이벤트를 구독 */
	virtual void InitializeForActor(AActor* InTargetActor, ULocalPlayer* InLocalPlayer) override;

	/** 컴포넌트 이벤트 구독 해제 및 내부 캐시 정리 */
	virtual void Deinitialize() override;

	/** 플레이어 인벤토리 설정 (TakeItem 시 아이템 이동 대상) */
	void SetPlayerInventory(UPBInventoryComponent* InPlayerInventory);

	/** 슬롯 인덱스의 아이템을 플레이어 인벤토리로 이동. 성공 여부 반환 */
	bool TakeItem(int32 SlotIndex);

	/** 루팅 가능한 모든 아이템을 플레이어 인벤토리로 이동 */
	void TakeAllItems();

	/** 루팅 슬롯 배열 반환 */
	const TArray<FPBLootSlotData>& GetLootSlots() const { return LootSlots; }

	/** 루팅 슬롯 수 반환 */
	UFUNCTION(BlueprintPure, Category = "UI|Loot")
	int32 GetLootSlotCount() const { return LootSlots.Num(); }

	/** 루팅 대상 이름 반환 */
	UFUNCTION(BlueprintPure, Category = "UI|Loot")
	FText GetOwnerName() const { return OwnerName; }

public:
	// 루팅 슬롯 전체 갱신 이벤트
	FOnLootSlotsRefreshed OnLootSlotsRefreshed;

protected:
	// 장비(장착 중) + 인벤토리 아이템 통합 슬롯 배열
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "UI|Loot")
	TArray<FPBLootSlotData> LootSlots;

private:
	/** 루팅 슬롯 배열을 장비 → 인벤토리 순으로 재구성 */
	void BuildLootSlots();

	/** FPBItemInstance를 FPBInventorySlotData로 변환 */
	FPBInventorySlotData BuildSlotData(const FPBItemInstance& ItemInstance) const;

	/** 아이템 등급에 대응하는 UI 색상 반환 */
	FLinearColor GetRarityColor(EPBItemRarity Rarity) const;

	/** 인벤토리 변경 이벤트 처리 (장비 해제 후 인벤토리 반영) */
	UFUNCTION()
	void HandleTargetInventoryChanged(int32 SlotIndex);

	/** 인벤토리 전체 갱신 이벤트 처리 */
	UFUNCTION()
	void HandleTargetInventoryFullRefresh();

	/** 장비 슬롯 변경 이벤트 처리 */
	UFUNCTION()
	void HandleTargetEquipmentChanged(EPBEquipSlot Slot);

private:
	// 루팅 대상의 인벤토리 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPBInventoryComponent> TargetInventory;

	// 루팅 대상의 장비 컴포넌트 (없을 수 있음)
	UPROPERTY(Transient)
	TObjectPtr<UPBEquipmentComponent> TargetEquipment;

	// 아이템 이동 목적지인 플레이어 인벤토리
	UPROPERTY(Transient)
	TObjectPtr<UPBInventoryComponent> PlayerInventory;

	// 루팅 대상 이름 (UI 표시용)
	FText OwnerName;

	// TakeItem 중 중간 갱신 억제 플래그
	bool bIsTakingItem = false;
};
