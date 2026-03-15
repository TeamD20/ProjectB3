// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBInventoryComponent.generated.h"

class UPBItemDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemChanged, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryFullRefresh);

// 캐릭터의 인벤토리(가방)를 관리하는 컴포넌트
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECTB3_API UPBInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBInventoryComponent();

	// 아이템 추가, 스택 가능 시 합산. 실제 추가된 수량 반환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UPBItemDataAsset* ItemData, int32 Amount = 1);

	// InstanceID로 아이템 제거. 성공 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(const FGuid& InstanceID, int32 Amount = 1);

	// 슬롯 인덱스로 아이템 인스턴스 조회
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FPBItemInstance GetItemAtSlot(int32 SlotIndex) const;

	// InstanceID로 아이템 인스턴스 조회
	FPBItemInstance FindItemByID(const FGuid& InstanceID) const;

	// InstanceID로 슬롯 인덱스 조회, 없으면 INDEX_NONE
	int32 FindSlotIndexByID(const FGuid& InstanceID) const;

	// 현재 사용 중인 슬롯 수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetUsedSlotCount() const { return Items.Num(); }

	// 빈 슬롯 존재 여부
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasFreeSlot() const { return Items.Num() < MaxSlots; }

	// 전체 아이템 배열 읽기 전용 접근
	const TArray<FPBItemInstance>& GetItems() const { return Items; }

	// 최대 슬롯 수
	int32 GetMaxSlots() const { return MaxSlots; }

private:
	// 동일 DataAsset의 기존 스택 슬롯 인덱스 검색 (합산 가능한 슬롯)
	int32 FindStackableSlotIndex(const UPBItemDataAsset* ItemData) const;

public:
	// 특정 슬롯 변경 시 Broadcast
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemChanged OnInventoryItemChanged;

	// 전체 갱신 시 Broadcast (정렬 등)
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryFullRefresh OnInventoryFullRefresh;

protected:
	// 인벤토리 아이템 리스트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FPBItemInstance> Items;

	// 인벤토리 슬롯 상한
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxSlots = 30;
};
