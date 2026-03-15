// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBItemSystemLibrary.generated.h"

class UPBInventoryComponent;
class UPBEquipmentComponent;
class UPBItemDataAsset;
class UPBEquipmentDataAsset;

// 아이템/인벤토리/장비 시스템 BP 접근용 정적 헬퍼
UCLASS()
class PROJECTB3_API UPBItemSystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ==== 컴포넌트 획득 ====

	// 액터에서 InventoryComponent 조회
	UFUNCTION(BlueprintPure, Category = "ItemSystem")
	static UPBInventoryComponent* GetInventoryComponent(const AActor* Actor);

	// 액터에서 EquipmentComponent 조회
	UFUNCTION(BlueprintPure, Category = "ItemSystem")
	static UPBEquipmentComponent* GetEquipmentComponent(const AActor* Actor);

	// ==== 장착 / 해제 ====

	// 지정 슬롯에 장착 (인벤토리 → 장비)
	UFUNCTION(BlueprintCallable, Category = "ItemSystem")
	static bool EquipItemToSlot(AActor* Actor, const FGuid& InstanceID, EPBEquipSlot Slot);

	// 자동 슬롯 결정 후 장착
	UFUNCTION(BlueprintCallable, Category = "ItemSystem")
	static bool AutoEquipItem(AActor* Actor, const FGuid& InstanceID);

	// 슬롯 장비 해제 (장비 → 인벤토리)
	UFUNCTION(BlueprintCallable, Category = "ItemSystem")
	static bool UnequipSlot(AActor* Actor, EPBEquipSlot Slot);

	// ==== 조회 ====

	// 해당 슬롯의 장착 아이템 조회
	UFUNCTION(BlueprintPure, Category = "ItemSystem")
	static FPBItemInstance GetEquippedItem(const AActor* Actor, EPBEquipSlot Slot);

	// 해당 슬롯이 비어있는지 확인
	UFUNCTION(BlueprintPure, Category = "ItemSystem")
	static bool IsSlotEmpty(const AActor* Actor, EPBEquipSlot Slot);

	// 아이템이 장착 가능한 장비인지 확인
	UFUNCTION(BlueprintPure, Category = "ItemSystem")
	static bool IsEquippableItem(const UPBItemDataAsset* ItemData);
};
