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
	// 장착 가능한 슬롯 부위 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<EPBEquipSlot> AllowedSlots;

	// 무기 손 점유 유형 (한손/양손/다용도). 무기 타입 아이템에만 유효.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",
		meta = (EditCondition = "ItemType == EPBItemType::Weapon", EditConditionHides))
	EPBWeaponHandType WeaponHandType = EPBWeaponHandType::OneHanded;

	// 장착 시 부여할 스탯 변경 이펙트 (방어도, 공격력 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<FPBEffectGrantEntry> GrantedEffects;

	// 장착 시 부여할 어빌리티 (특수 주문, 패시브 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<FPBAbilityGrantEntry> GrantedAbilities;

	// 캐릭터 메시에 부착할 장비 액터 클래스 (무기 메시/애님 레이어, 장착 시 로드)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Visual")
	TSoftClassPtr<APBEquipmentActor> EquipmentActorClass;

public:
	// 해당 슬롯에 장착 가능한지 검증
	bool CanEquipToSlot(EPBEquipSlot Slot) const
	{
		return AllowedSlots.Contains(Slot);
	}
};
