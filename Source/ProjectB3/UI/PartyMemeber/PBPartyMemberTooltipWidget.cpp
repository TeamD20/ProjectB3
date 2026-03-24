// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPartyMemberTooltipWidget.h"
#include "PBPartyMemberViewModel.h"
#include "PBPartyMemberTooltipRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"

void UPBPartyMemberTooltipWidget::InitializeTooltip(UPBPartyMemberViewModel* ViewModel)
{
	if (!IsValid(ViewModel))
	{
		return;
	}

	MemberViewModel = ViewModel;

	ViewModel->OnNameChanged.AddUObject(this, &ThisClass::HandleNameChanged);
	ViewModel->OnLevelChanged.AddUObject(this, &ThisClass::HandleLevelChanged);
	ViewModel->OnClassChanged.AddUObject(this, &ThisClass::HandleClassChanged);
	ViewModel->OnClassIconChanged.AddUObject(this, &ThisClass::HandleClassIconChanged);
	ViewModel->OnBuffsChanged.AddUObject(this, &ThisClass::HandleBuffsChanged);
	ViewModel->OnDebuffsChanged.AddUObject(this, &ThisClass::HandleDebuffsChanged);

	RefreshUI();
}

void UPBPartyMemberTooltipWidget::RefreshUI()
{
	if (!IsValid(MemberViewModel))
	{
		return;
	}

	HandleNameChanged(MemberViewModel->GetCharacterName());
	HandleLevelChanged(MemberViewModel->GetCharacterLevel());
	HandleClassChanged(MemberViewModel->GetCharacterClass());
	HandleClassIconChanged(MemberViewModel->GetClassIcon());
	HandleBuffsChanged();
	HandleDebuffsChanged();
}

void UPBPartyMemberTooltipWidget::NativeDestruct()
{
	if (IsValid(MemberViewModel))
	{
		MemberViewModel->OnNameChanged.RemoveAll(this);
		MemberViewModel->OnLevelChanged.RemoveAll(this);
		MemberViewModel->OnClassChanged.RemoveAll(this);
		MemberViewModel->OnClassIconChanged.RemoveAll(this);
		MemberViewModel->OnBuffsChanged.RemoveAll(this);
		MemberViewModel->OnDebuffsChanged.RemoveAll(this);
		MemberViewModel = nullptr;
	}

	Super::NativeDestruct();
}

void UPBPartyMemberTooltipWidget::HandleNameChanged(FText InName)
{
	if (IsValid(CharacterNameTextBlock))
	{
		// 이름은 가공 없이 그대로 표시 ("Name : X" 제거)
		CharacterNameTextBlock->SetText(InName);
	}
}

void UPBPartyMemberTooltipWidget::HandleLevelChanged(FText InLevel)
{
	if (IsValid(CharacterLevelTextBlock))
	{
		FText Formatted = FText::Format(NSLOCTEXT("UI", "TooltipLevel", "Lv. {0}"), InLevel);
		CharacterLevelTextBlock->SetText(Formatted);
	}
}

void UPBPartyMemberTooltipWidget::HandleClassChanged(FText InClass)
{
	if (IsValid(CharacterClassTextBlock))
	{
		FText Formatted = FText::Format(NSLOCTEXT("UI", "TooltipClass", "Class. {0}"), InClass);
		CharacterClassTextBlock->SetText(Formatted);
	}
}

void UPBPartyMemberTooltipWidget::HandleLevelAndClassChanged()
{
	if (!IsValid(MemberViewModel))
	{
		return;
	}
	HandleLevelChanged(MemberViewModel->GetCharacterLevel());
	HandleClassChanged(MemberViewModel->GetCharacterClass());
}

void UPBPartyMemberTooltipWidget::HandleClassIconChanged(TSoftObjectPtr<UTexture2D> InIcon)
{
	if (ClassIconImage && !InIcon.IsNull())
	{
		ClassIconImage->SetBrushFromSoftTexture(InIcon);
	}
}

void UPBPartyMemberTooltipWidget::HandleBuffsChanged()
{
	if (!IsValid(BuffBox) || !IsValid(MemberViewModel) || !RowWidgetClass)
	{
		return;
	}

	BuffBox->ClearChildren();

	for (const FPBPartyTooltipRowData& RowData : MemberViewModel->GetBuffs())
	{
		UPBPartyMemberTooltipRowWidget* RowWidget = CreateWidget<UPBPartyMemberTooltipRowWidget>(GetWorld(), RowWidgetClass);
		if (RowWidget)
		{
			RowWidget->InitializeRowData(RowData.Icon, RowData.Text);
			BuffBox->AddChild(RowWidget);
		}
	}

	BuffBox->SetVisibility(MemberViewModel->GetBuffs().Num() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UPBPartyMemberTooltipWidget::HandleDebuffsChanged()
{
	if (!IsValid(DeBuffBox) || !IsValid(MemberViewModel) || !RowWidgetClass)
	{
		return;
	}

	DeBuffBox->ClearChildren();

	for (const FPBPartyTooltipRowData& RowData : MemberViewModel->GetDebuffs())
	{
		UPBPartyMemberTooltipRowWidget* RowWidget = CreateWidget<UPBPartyMemberTooltipRowWidget>(GetWorld(), RowWidgetClass);
		if (RowWidget)
		{
			RowWidget->InitializeRowData(RowData.Icon, RowData.Text);
			DeBuffBox->AddChild(RowWidget);
		}
	}

	DeBuffBox->SetVisibility(MemberViewModel->GetDebuffs().Num() > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
