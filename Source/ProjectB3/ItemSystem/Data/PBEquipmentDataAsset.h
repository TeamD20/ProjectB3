// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBItemDataAsset.h"
#include "ProjectB3/AbilitySystem/PBAbilityGrantTypes.h"
#include "PBEquipmentDataAsset.generated.h"

class APBEquipmentActor;

// 장비 전용 DataAsset — UPBItemDataAsset 서브클래스
// DA_Equip_Longsword, DA_Equip_ChainMail 등
UCLASS(BlueprintType)
class PROJECTB3_API UPBEquipmentDataAsset : public UPBItemDataAsset
{
	GENERATED_BODY()

public:
	// 장비 부위 유형 — 허용 슬롯은 코드에서 자동 도출
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	EPBEquipmentType EquipmentType = EPBEquipmentType::Weapon;

	// 무기 손 점유 유형 (한손/양손/다용도). EquipmentType == Weapon 일 때만 유효.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment",
		meta = (EditCondition = "EquipmentType == EPBEquipmentType::Weapon", EditConditionHides))
	EPBWeaponHandType WeaponHandType = EPBWeaponHandType::OneHanded;

	// 장착 시 부여할 스탯 변경 이펙트 (방어도, 공격력 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	TArray<FPBEffectGrantEntry> GrantedEffects;

	// 장착 시 부여할 어빌리티 (특수 주문, 패시브 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment")
	TArray<FPBAbilityGrantEntry> GrantedAbilities;

	// 캐릭터 메시에 부착할 장비 액터 클래스 (무기 메시/애님 레이어, 장착 시 로드)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Equipment|Visual")
	TSoftClassPtr<APBEquipmentActor> EquipmentActorClass;

	// 장비 메시 부착 위치 Override
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FGameplayTag AttachSlotOverride;
	
public:
	// EquipmentType과 WeaponHandType으로부터 허용 슬롯 목록을 도출
	static TArray<EPBEquipSlot> GetAllowedSlotsForType(EPBEquipmentType InEquipmentType, EPBWeaponHandType InHandType);

	// 해당 슬롯에 장착 가능한지 검증
	bool CanEquipToSlot(EPBEquipSlot Slot) const
	{
		return GetAllowedSlotsForType(EquipmentType, WeaponHandType).Contains(Slot);
	}
};
