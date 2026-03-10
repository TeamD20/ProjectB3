// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillSlotWidget.h"
#include "PBSkillBarViewModel.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"

void UPBSkillSlotWidget::SetSlotData(const FPBSkillSlotData& InSlotData)
{
	SlotData = InSlotData;

	if (IsValid(IconImage))
	{
		IconImage->SetBrushFromSoftTexture(SlotData.Icon);
	}

	if (IsValid(CooldownText))
	{
		const FText CooldownValueText = SlotData.CooldownRemaining > 0
			? FText::AsNumber(SlotData.CooldownRemaining)
			: FText::GetEmpty();
		CooldownText->SetText(CooldownValueText);
	}

	if (IsValid(CooldownOverlay))
	{
		const bool bShowOverlay = SlotData.CooldownRemaining > 0;
		CooldownOverlay->SetVisibility(bShowOverlay ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (IsValid(SlotButton))
	{
		SlotButton->SetIsEnabled(SlotData.bCanActivate && SlotData.CooldownRemaining <= 0);
	}
}

void UPBSkillSlotWidget::SetSlotIndex(int32 InTabIndex, int32 InSlotIndex)
{
	TabIndex = InTabIndex;
	SlotIndex = InSlotIndex;
}

void UPBSkillSlotWidget::InitializeBinding(UPBSkillBarViewModel* InSkillBarViewModel)
{
	SkillBarViewModel = InSkillBarViewModel;
}

void UPBSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.AddDynamic(this, &UPBSkillSlotWidget::OnSlotClicked);
	}
}

void UPBSkillSlotWidget::NativeDestruct()
{
	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPBSkillSlotWidget::OnSlotClicked);
	}

	SkillBarViewModel.Reset();

	Super::NativeDestruct();
}

void UPBSkillSlotWidget::OnSlotClicked()
{
	if (!SkillBarViewModel.IsValid())
	{
		return;
	}

	FPBSkillSlotData CurrentData;
	if (!SkillBarViewModel->GetSlotData(TabIndex, SlotIndex, CurrentData))
	{
		return;
	}

	APBGameplayPlayerState* PlayerState = SkillBarViewModel->GetPlayerState();
	if (!IsValid(PlayerState))
	{
		return;
	}

	PlayerState->RequestAbilityActivation(CurrentData.AbilityHandle);
}
