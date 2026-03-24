// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillBarWidget.h"
#include "PBSkillSlotWidget.h"
#include "PBSkillBarViewModel.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "Components/PanelWidget.h"
#include "ProjectB3/UI/PBUITypes.h"

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

void UPBSkillBarWidget::RefreshSkillBar()
{
	if (!IsValid(SkillBarViewModel))
	{
		return;
	}

	auto RebuildContainer = [this](UPanelWidget* Container, const TArray<FPBSkillSlotData>& Slots, int32 CategoryIndex, int32 MinSlots)
	{
		if (!IsValid(Container))
		{
			return;
		}

		Container->ClearChildren();

		const int32 SlotCount = FMath::Max(Slots.Num(), MinSlots);
		for (int32 i = 0; i < SlotCount; ++i)
		{
			if (UPBSkillSlotWidget* SlotWidget = CreateWidget<UPBSkillSlotWidget>(this, SkillSlotWidgetClass))
			{
				SlotWidget->InitializeBinding(SkillBarViewModel.Get());
				SlotWidget->SetSlotIndex(CategoryIndex, i);

				// 할당된 스킬이 있으면 데이터 설정, 없으면 빈 슬롯
				if (Slots.IsValidIndex(i))
				{
					SlotWidget->SetSlotData(Slots[i]);
				}
				else
				{
					SlotWidget->SetSlotData(FPBSkillSlotData());
				}

				Container->AddChild(SlotWidget);
			}
		}
	};

	RebuildContainer(PrimaryActionContainer, SkillBarViewModel->PrimaryActions, 0, MinPrimarySlots);
	RebuildContainer(SecondaryActionContainer, SkillBarViewModel->SecondaryActions, 1, MinSecondarySlots);
	RebuildContainer(ConsumableContainer, SkillBarViewModel->ConsumableActions, 2, MinConsumableSlots);
}

void UPBSkillBarWidget::HandleSlotUpdated(int32 CategoryIndex, int32 SlotIndex)
{
	if (!IsValid(SkillBarViewModel))
	{
		return;
	}

	UPanelWidget* TargetContainer = nullptr;
	switch (CategoryIndex)
	{
	case 0: TargetContainer = PrimaryActionContainer; break;
	case 1: TargetContainer = SecondaryActionContainer; break;
	case 2: TargetContainer = ConsumableContainer; break;
	}

	if (IsValid(TargetContainer) && TargetContainer->GetChildrenCount() > SlotIndex)
	{
		if (UPBSkillSlotWidget* SlotWidget = Cast<UPBSkillSlotWidget>(TargetContainer->GetChildAt(SlotIndex)))
		{
			FPBSkillSlotData UpdatedData;
			if (SkillBarViewModel->GetSlotData(CategoryIndex, SlotIndex, UpdatedData))
			{
				SlotWidget->SetSlotData(UpdatedData);
			}
		}
	}
}
