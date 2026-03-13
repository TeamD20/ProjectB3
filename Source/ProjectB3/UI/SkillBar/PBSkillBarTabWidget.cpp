// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillBarTabWidget.h"
#include "PBSkillSlotWidget.h"
#include "PBSkillBarViewModel.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "Components/PanelWidget.h"

void UPBSkillBarTabWidget::RebuildSlots(const TArray<FPBSkillSlotData>& Slots, UPBSkillBarViewModel* ViewModel, int32 InTabIndex, int32 MaxSlotCount, TSubclassOf<UPBSkillSlotWidget> SlotWidgetClass)
{
	TabIndex = InTabIndex;

	if (!IsValid(SlotContainer) || !IsValid(SlotWidgetClass))
	{
		return;
	}

	SlotContainer->ClearChildren();

	for (int32 Index = 0; Index < MaxSlotCount; ++Index)
	{
		UPBSkillSlotWidget* SlotWidget = CreateWidget<UPBSkillSlotWidget>(GetWorld(), SlotWidgetClass);
		if (!IsValid(SlotWidget))
		{
			continue;
		}

		SlotWidget->InitializeBinding(ViewModel);
		SlotWidget->SetSlotIndex(TabIndex, Index);

		if (Slots.IsValidIndex(Index))
		{
			SlotWidget->SetSlotData(Slots[Index]);
		}

		SlotContainer->AddChild(SlotWidget);
	}
}

void UPBSkillBarTabWidget::UpdateSlot(int32 SlotIndex, const FPBSkillSlotData& SlotData)
{
	if (!IsValid(SlotContainer) || !SlotContainer->GetAllChildren().IsValidIndex(SlotIndex))
	{
		return;
	}

	UPBSkillSlotWidget* SlotWidget = Cast<UPBSkillSlotWidget>(SlotContainer->GetChildAt(SlotIndex));
	if (IsValid(SlotWidget))
	{
		SlotWidget->SetSlotData(SlotData);
	}
}
