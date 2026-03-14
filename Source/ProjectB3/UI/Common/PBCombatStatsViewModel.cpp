// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/Common/PBCombatStatsViewModel.h"

void UPBCombatStatsViewModel::SetMovement(float InCurrent, float InMax)
{
	if (FMath::IsNearlyEqual(CurrentMovement, InCurrent) && FMath::IsNearlyEqual(MaxMovement, InMax))
	{
		return;
	}

	CurrentMovement = InCurrent;
	MaxMovement = InMax;
	MovementPercent = (MaxMovement > 0.f) ? FMath::Clamp(CurrentMovement / MaxMovement, 0.f, 1.f) : 0.f;

	OnMovementChanged.Broadcast(CurrentMovement, MaxMovement);
	OnMovementPercentChanged.Broadcast(MovementPercent);
}

void UPBCombatStatsViewModel::SetArmorClass(int32 InAC)
{
	if (ArmorClass == InAC)
	{
		return;
	}

	ArmorClass = InAC;
	OnArmorClassChanged.Broadcast(ArmorClass);
}

void UPBCombatStatsViewModel::SetHitBonus(int32 InHitBonus)
{
	if (HitBonus == InHitBonus)
	{
		return;
	}

	HitBonus = InHitBonus;
	OnHitBonusChanged.Broadcast(HitBonus);
}

void UPBCombatStatsViewModel::SetSpellSaveDC(int32 InDC)
{
	if (SpellSaveDC == InDC)
	{
		return;
	}

	SpellSaveDC = InDC;
	OnSpellSaveDCChanged.Broadcast(SpellSaveDC);
}
