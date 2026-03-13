// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/MainActionBar/PBMainActionBarHUD.h"
#include "ProjectB3/UI/SkillBar/PBSkillBarViewModel.h"
#include "ProjectB3/UI/SkillBar/PBEquipmentSlotWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "Components/PanelWidget.h"

void UPBMainActionBarHUD::NativeConstruct()
{
	Super::NativeConstruct();
	
	SkillBarViewModel = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBSkillBarViewModel>(GetOwningLocalPlayer());
	if (IsValid(SkillBarViewModel))
	{
		SlotsChangedHandle = SkillBarViewModel->OnSlotsChanged.AddUObject(this, &UPBMainActionBarHUD::HandleSlotsChanged);
		RefreshEquipmentSlots();
	}
}

void UPBMainActionBarHUD::NativeDestruct()
{
	if (IsValid(SkillBarViewModel))
	{
		if (SlotsChangedHandle.IsValid())
		{
			SkillBarViewModel->OnSlotsChanged.Remove(SlotsChangedHandle);
			SlotsChangedHandle.Reset();
		}
	}

	SkillBarViewModel = nullptr;

	Super::NativeDestruct();
}

void UPBMainActionBarHUD::HandleSlotsChanged()
{
	RefreshEquipmentSlots();
}

void UPBMainActionBarHUD::RefreshEquipmentSlots()
{
	if (!IsValid(SkillBarViewModel) || !IsValid(EquipmentSlotWidgetClass))
	{
		return;
	}

	auto RebuildContainer = [this](UPanelWidget* Container, const TArray<FPBEquipmentSlotData>& Slots)
	{
		if (!IsValid(Container)) return;
		
		Container->ClearChildren();

		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (UPBEquipmentSlotWidget* SlotWidget = CreateWidget<UPBEquipmentSlotWidget>(this, EquipmentSlotWidgetClass))
			{
				SlotWidget->UpdateSlot(Slots[i]);
				Container->AddChild(SlotWidget);
			}
		}
	};

	RebuildContainer(MainWeaponSlots, SkillBarViewModel->WeaponSlots);
	RebuildContainer(UtilitySlots, SkillBarViewModel->UtilitySlots);
}
