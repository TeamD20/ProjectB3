// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPartyMemberTooltipWidget.h"
#include "PBPartyMemberViewModel.h"
#include "PBPartyMemberTooltipRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

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
	ViewModel->OnStatusEffectsChanged.AddUObject(this, &ThisClass::HandleStatusEffectsChanged);
	ViewModel->OnReactionsChanged.AddUObject(this, &ThisClass::HandleReactionsChanged);

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
	HandleStatusEffectsChanged();
	HandleReactionsChanged();
}

void UPBPartyMemberTooltipWidget::NativeDestruct()
{
	if (IsValid(MemberViewModel))
	{
		MemberViewModel->OnNameChanged.RemoveAll(this);
		MemberViewModel->OnLevelChanged.RemoveAll(this);
		MemberViewModel->OnClassChanged.RemoveAll(this);
		MemberViewModel->OnStatusEffectsChanged.RemoveAll(this);
		MemberViewModel->OnReactionsChanged.RemoveAll(this);
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

void UPBPartyMemberTooltipWidget::HandleStatusEffectsChanged()
{
	if (!IsValid(StatusEffectsContainer) || !IsValid(MemberViewModel) || !RowWidgetClass)
	{
		return;
	}

	StatusEffectsContainer->ClearChildren();

	for (const FPBPartyTooltipRowData& RowData : MemberViewModel->GetStatusEffects())
	{
		UPBPartyMemberTooltipRowWidget* RowWidget = CreateWidget<UPBPartyMemberTooltipRowWidget>(GetWorld(), RowWidgetClass);
		if (RowWidget)
		{
			RowWidget->InitializeRowData(RowData.Icon, RowData.Text);
			StatusEffectsContainer->AddChildToVerticalBox(RowWidget);
		}
	}
}

void UPBPartyMemberTooltipWidget::HandleReactionsChanged()
{
	if (!IsValid(ReactionsContainer) || !IsValid(MemberViewModel) || !RowWidgetClass)
	{
		return;
	}

	ReactionsContainer->ClearChildren();

	for (const FPBPartyTooltipRowData& RowData : MemberViewModel->GetReactions())
	{
		UPBPartyMemberTooltipRowWidget* RowWidget = CreateWidget<UPBPartyMemberTooltipRowWidget>(GetWorld(), RowWidgetClass);
		if (RowWidget)
		{
			RowWidget->InitializeRowData(RowData.Icon, RowData.Text);
			ReactionsContainer->AddChildToVerticalBox(RowWidget);
		}
	}
}
