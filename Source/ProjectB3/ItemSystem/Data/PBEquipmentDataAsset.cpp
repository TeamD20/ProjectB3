// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEquipmentDataAsset.h"

FPBDiceSpec UPBEquipmentDataAsset::GetAbilityDamageSpec() const
{
	return DamageSpec;
}

int32 UPBEquipmentDataAsset::GetWeaponBonusModifier() const
{
	return WeaponBonusModifier;
}

FGameplayTag UPBEquipmentDataAsset::GetDamageTypeTag() const
{
	return DamageTypeTag;
}

int32 UPBEquipmentDataAsset::GetArmorClass() const
{
	return ArmorClass;
}TArray<EPBEquipSlot> UPBEquipmentDataAsset::GetAllowedSlotsForType(EPBEquipmentType InEquipmentType, EPBWeaponHandType InHandType)
{
	switch (InEquipmentType)
	{
	case EPBEquipmentType::Weapon:
		// 양손무기: Main 슬롯만 허용
		if (InHandType == EPBWeaponHandType::TwoHanded)
		{
			return { EPBEquipSlot::WeaponSet1_Main, EPBEquipSlot::WeaponSet2_Main };
		}
		// 한손/다용도: Main + Off 모두 허용
		return {
			EPBEquipSlot::WeaponSet1_Main, EPBEquipSlot::WeaponSet1_Off,
			EPBEquipSlot::WeaponSet2_Main, EPBEquipSlot::WeaponSet2_Off
		};

	case EPBEquipmentType::Shield:
		return { EPBEquipSlot::WeaponSet1_Off, EPBEquipSlot::WeaponSet2_Off };

	case EPBEquipmentType::Head:
		return { EPBEquipSlot::Head };

	case EPBEquipmentType::Body:
		return { EPBEquipSlot::Body };

	case EPBEquipmentType::Hands:
		return { EPBEquipSlot::Hands };

	case EPBEquipmentType::Feet:
		return { EPBEquipSlot::Feet };

	case EPBEquipmentType::Amulet:
		return { EPBEquipSlot::Amulet };

	case EPBEquipmentType::Ring:
		return { EPBEquipSlot::Ring1, EPBEquipSlot::Ring2 };

	default:
		return {};
	}
}
