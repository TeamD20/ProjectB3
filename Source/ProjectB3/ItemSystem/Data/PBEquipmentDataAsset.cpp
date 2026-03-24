// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEquipmentDataAsset.h"

FPBDiceSpec UPBEquipmentDataAsset::GetAbilityDamageSpec() const
{
	// TODO: 동적 데이터 연동 전 더미값 반환
	FPBDiceSpec DummySpec;
	DummySpec.DiceCount = 1;
	DummySpec.DiceFaces = 6;
	return DummySpec;
}

int32 UPBEquipmentDataAsset::GetWeaponBonusModifier() const
{
	// TODO: 동적 데이터 연동 전 더미값 반환
	return 0;
}

FGameplayTag UPBEquipmentDataAsset::GetDamageTypeTag() const
{
	// TODO: 동적 데이터 연동 전 더미값 반환
	return FGameplayTag::EmptyTag;
}

int32 UPBEquipmentDataAsset::GetArmorClass() const
{
	// TODO: 동적 데이터 연동 전 더미값 반환
	return 10;
}

TArray<EPBEquipSlot> UPBEquipmentDataAsset::GetAllowedSlotsForType(EPBEquipmentType InEquipmentType, EPBWeaponHandType InHandType)
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
