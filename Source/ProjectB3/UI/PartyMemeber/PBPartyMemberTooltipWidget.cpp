// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPartyMemberTooltipWidget.h"
#include "PBPartyMemberViewModel.h"
#include "Components/TextBlock.h"

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
}

void UPBPartyMemberTooltipWidget::NativeDestruct()
{
	if (IsValid(MemberViewModel))
	{
		MemberViewModel->OnNameChanged.RemoveAll(this);
		MemberViewModel->OnLevelChanged.RemoveAll(this);
		MemberViewModel->OnClassChanged.RemoveAll(this);
		MemberViewModel = nullptr;
	}

	Super::NativeDestruct();
}

void UPBPartyMemberTooltipWidget::HandleNameChanged(FText InName)
{
	if (IsValid(CharacterNameTextBlock))
	{
		FText FormattedName = FText::Format(NSLOCTEXT("UI", "TooltipName", "Name : {0}"), InName);
		CharacterNameTextBlock->SetText(FormattedName);
	}
}

void UPBPartyMemberTooltipWidget::HandleLevelChanged(FText InLevel)
{
	if (IsValid(CharacterLevelTextBlock))
	{
		FText FormattedLevel = FText::Format(NSLOCTEXT("UI", "TooltipLevel", "Lv. {0}"), InLevel);
		CharacterLevelTextBlock->SetText(FormattedLevel);
	}
}

void UPBPartyMemberTooltipWidget::HandleClassChanged(FText InClass)
{
	if (IsValid(CharacterClassTextBlock))
	{
		FText FormattedClass = FText::Format(NSLOCTEXT("UI", "TooltipClass", " / Class : {0}"), InClass);
		CharacterClassTextBlock->SetText(FormattedClass);
	}
}
