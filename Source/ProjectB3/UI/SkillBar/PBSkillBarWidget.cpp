// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillBarWidget.h"
#include "PBSkillBarViewModel.h"
#include "PBSkillSlotWidget.h"
#include "PBSkillBarTypes.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

namespace PBSkillBarTabIndex
{
	static constexpr int32 Action = 0;
	static constexpr int32 BonusAction = 1;
	static constexpr int32 Spell = 2;
}

void UPBSkillBarWidget::SetCurrentTab(int32 InTabIndex)
{
	if (IsValid(TabSwitcher))
	{
		TabSwitcher->SetActiveWidgetIndex(InTabIndex);
	}
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

	HandleSlotsChanged();
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
	if (!IsValid(SkillBarViewModel))
	{
		return;
	}

	RebuildSlots(ActionSlotContainer, SkillBarViewModel->ActionSlots, PBSkillBarTabIndex::Action, ActionSlotCount);
	RebuildSlots(BonusActionSlotContainer, SkillBarViewModel->BonusActionSlots, PBSkillBarTabIndex::BonusAction, BonusActionSlotCount);
	RebuildSlots(SpellSlotContainer, SkillBarViewModel->SpellSlots, PBSkillBarTabIndex::Spell, SpellSlotCount);
}

void UPBSkillBarWidget::HandleSlotUpdated(int32 TabIndex, int32 SlotIndex)
{
	if (!IsValid(SkillBarViewModel))
	{
		return;
	}

	UPanelWidget* TargetContainer = GetContainerByTab(TabIndex);
	if (!IsValid(TargetContainer) || !TargetContainer->GetAllChildren().IsValidIndex(SlotIndex))
	{
		return;
	}

	UPBSkillSlotWidget* SlotWidget = Cast<UPBSkillSlotWidget>(TargetContainer->GetChildAt(SlotIndex));
	if (!IsValid(SlotWidget))
	{
		return;
	}

	FPBSkillSlotData UpdatedSlotData;
	if (SkillBarViewModel->GetSlotData(TabIndex, SlotIndex, UpdatedSlotData))
	{
		SlotWidget->SetSlotData(UpdatedSlotData);
	}
}

void UPBSkillBarWidget::RebuildSlots(UPanelWidget* Container, const TArray<FPBSkillSlotData>& Slots, int32 TabIndex, int32 MaxSlotCount)
{
	if (!IsValid(Container) || !IsValid(SkillSlotWidgetClass))
	{
		return;
	}

	Container->ClearChildren();

	// MaxSlotCount만큼 항상 슬롯 위젯을 생성한다. 데이터가 없는 인덱스는 빈 슬롯으로 표시된다.
	for (int32 SlotIndex = 0; SlotIndex < MaxSlotCount; ++SlotIndex)
	{
		UPBSkillSlotWidget* SlotWidget = CreateWidget<UPBSkillSlotWidget>(GetWorld(), SkillSlotWidgetClass);
		if (!IsValid(SlotWidget))
		{
			continue;
		}

		SlotWidget->InitializeBinding(SkillBarViewModel);
		SlotWidget->SetSlotIndex(TabIndex, SlotIndex);

		if (Slots.IsValidIndex(SlotIndex))
		{
			SlotWidget->SetSlotData(Slots[SlotIndex]);
		}

		Container->AddChild(SlotWidget);
	}
}

UPanelWidget* UPBSkillBarWidget::GetContainerByTab(int32 TabIndex) const
{
	switch (TabIndex)
	{
	case PBSkillBarTabIndex::Action:
		return ActionSlotContainer;
	case PBSkillBarTabIndex::BonusAction:
		return BonusActionSlotContainer;
	case PBSkillBarTabIndex::Spell:
		return SpellSlotContainer;
	default:
		return nullptr;
	}
}
