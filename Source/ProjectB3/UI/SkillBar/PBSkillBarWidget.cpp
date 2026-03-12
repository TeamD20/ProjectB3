// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillBarWidget.h"
#include "PBSkillIconWidget.h"
#include "PBSkillBarViewModel.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "Components/PanelWidget.h"
#include "ProjectB3/UI/PBUITypes.h"


void UPBSkillBarWidget::RefreshSkillBar()
{
	if (!IsValid(SkillBarViewModel)) return;

	auto RebuildContainer = [this](UPanelWidget* Container, const TArray<FPBSkillSlotData>& Slots)
	{
		if (!IsValid(Container)) return;
		
		Container->ClearChildren();
		for (const FPBSkillSlotData& SlotData : Slots)
		{
			if (UPBSkillIconWidget* IconWidget = CreateWidget<UPBSkillIconWidget>(this, SkillIconWidgetClass))
			{
				IconWidget->UpdateSlot(SlotData);
				Container->AddChild(IconWidget);
			}
		}
	};

	RebuildContainer(PrimaryActionContainer, SkillBarViewModel->PrimaryActions);
	RebuildContainer(SecondaryActionContainer, SkillBarViewModel->SecondaryActions);
	RebuildContainer(ItemSlotContainer, SkillBarViewModel->ItemSlots);
}

void UPBSkillBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SkillBarViewModel = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBSkillBarViewModel>(GetOwningLocalPlayer());
	if (!IsValid(SkillBarViewModel))
	{
		return;
	}

	SlotsChangedHandle = SkillBarViewModel->OnSlotsChanged.AddUObject(this, &UPBSkillBarWidget::HandleSlotsChanged);
	SlotUpdatedHandle = SkillBarViewModel->OnSlotUpdated.AddUObject(this, &UPBSkillBarWidget::HandleSlotUpdated);

	RefreshSkillBar();
}

void UPBSkillBarWidget::NativeDestruct()
{
	if (IsValid(SkillBarViewModel))
	{
		if (SlotsChangedHandle.IsValid())
		{
			SkillBarViewModel->OnSlotsChanged.Remove(SlotsChangedHandle);
			SlotsChangedHandle.Reset();
		}

		if (SlotUpdatedHandle.IsValid())
		{
			SkillBarViewModel->OnSlotUpdated.Remove(SlotUpdatedHandle);
			SlotUpdatedHandle.Reset();
		}
	}

	SkillBarViewModel = nullptr;

	Super::NativeDestruct();
}

void UPBSkillBarWidget::HandleSlotsChanged()
{
	RefreshSkillBar();
}

void UPBSkillBarWidget::HandleSlotUpdated(int32 CategoryIndex, int32 SlotIndex)
{
	if (!IsValid(SkillBarViewModel)) return;

	UPanelWidget* TargetContainer = nullptr;
	switch (CategoryIndex)
	{
	case 0: TargetContainer = PrimaryActionContainer; break;
	case 1: TargetContainer = SecondaryActionContainer; break;
	case 2: TargetContainer = ItemSlotContainer; break;
	}

	if (IsValid(TargetContainer) && TargetContainer->GetChildrenCount() > SlotIndex)
	{
		if (UPBSkillIconWidget* IconWidget = Cast<UPBSkillIconWidget>(TargetContainer->GetChildAt(SlotIndex)))
		{
			FPBSkillSlotData UpdatedData;
			if (SkillBarViewModel->GetSlotData(CategoryIndex, SlotIndex, UpdatedData))
			{
				IconWidget->UpdateSlot(UpdatedData);
			}
		}
	}
}
