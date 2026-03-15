// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/AbilitySystem/PBAbilityGrantTypes.h"
#include "PBEquipmentComponent.generated.h"

class UPBInventoryComponent;
class UPBEquipmentDataAsset;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotChanged, EPBEquipSlot, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponSetSwitched, int32, NewActiveSet);

// 캐릭터의 장비 슬롯을 관리하는 컴포넌트
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECTB3_API UPBEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBEquipmentComponent();

	// 아이템을 지정 슬롯에 장착 (인벤토리에서 제거)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItem(const FGuid& InstanceID, EPBEquipSlot Slot, UPBInventoryComponent* Inventory);

	// 슬롯의 장비 해제 (인벤토리 복귀)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipItem(EPBEquipSlot Slot, UPBInventoryComponent* Inventory);

	// 무기 세트 전환 (1↔2)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void SwitchWeaponSet();

	// 특정 슬롯의 장착 아이템 조회
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	FPBItemInstance GetEquippedItem(EPBEquipSlot Slot) const;

	// 슬롯이 비어있는지 확인
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool IsSlotEmpty(EPBEquipSlot Slot) const;

	// 현재 활성 무기 세트 번호 반환
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	int32 GetActiveWeaponSet() const { return ActiveWeaponSet; }

	// 전체 장착 맵 읽기 전용 접근
	const TMap<EPBEquipSlot, FPBItemInstance>& GetEquippedItems() const { return EquippedItems; }

	// 아이템의 AllowedSlots와 현재 장착 상태 기반으로 최적 슬롯 자동 결정
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	EPBEquipSlot ResolveAutoEquipSlot(const UPBEquipmentDataAsset* EquipData) const;

	// 자동 슬롯 결정 후 장착 (우클릭 장착, 더블클릭 등)
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool AutoEquipItem(const FGuid& InstanceID, UPBInventoryComponent* Inventory);

	// 슬롯에 대응하는 부착 위치 태그 반환 (Main→RightHand, Off→LeftHand)
	UFUNCTION(BlueprintPure, Category = "Equipment")
	static FGameplayTag GetAttachSlotTag(EPBEquipSlot Slot);

private:
	// 장착 시 GAS에 이펙트/어빌리티 부여
	void GrantEquipmentAbilities(EPBEquipSlot Slot, const UPBEquipmentDataAsset* EquipData);

	// 해제 시 GAS에서 이펙트/어빌리티 제거
	void RevokeEquipmentAbilities(EPBEquipSlot Slot);

	// 소유자의 ASC 획득
	UAbilitySystemComponent* GetOwnerASC() const;

	// 슬롯이 무기 세트에 속하는지 확인
	static bool IsWeaponSlot(EPBEquipSlot Slot);

	// 슬롯이 특정 무기 세트에 속하는지 확인
	static int32 GetWeaponSetNumber(EPBEquipSlot Slot);

public:
	// 장비 슬롯 변경 시 Broadcast
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentSlotChanged OnEquipmentSlotChanged;

	// 무기 세트 전환 시 Broadcast
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnWeaponSetSwitched OnWeaponSetSwitched;

protected:
	// 장착 슬롯별 아이템 매핑
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TMap<EPBEquipSlot, FPBItemInstance> EquippedItems;

	// 현재 활성 무기 세트 (1 또는 2)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	int32 ActiveWeaponSet = 1;

	// 슬롯별 부여된 GAS 핸들 (해제 시 정리용)
	TMap<EPBEquipSlot, FPBAbilityGrantedHandles> GrantedHandlesMap;

};
