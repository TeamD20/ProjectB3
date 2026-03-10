// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBPartyMemberWidget.h"
#include "PBPartyMemberViewModel.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"



void UPBPartyMemberWidget::UpdataHPText()
{
	if (IsValid(MemberViewModel) && IsValid(CharacterHPTextBlock))
	{
		FText HPText = MemberViewModel->GetCharacterHPText();
		CharacterHPTextBlock->SetText(HPText);
	}
}

#include "ProjectB3/UI/Common/PBPortraitBaseWidget.h"

void UPBPartyMemberWidget::HandleImageChanged(TSoftObjectPtr<UTexture2D> InPortrait)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetPortraitImage(InPortrait);
	}
}

void UPBPartyMemberWidget::HandleCharacterSelected(bool bInMyTurn)
{
			if (!SelectedOverlay)
	{
		return;
	}
	
	if (bInMyTurn)
	{
		SelectedOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		SelectedOverlay->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPBPartyMemberWidget::InitializeWithViewModel(UPBPartyMemberViewModel* ViewModel)
{
	if (!IsValid(ViewModel))
	{
		return;
	}
	
	MemberViewModel = ViewModel;
	
	ViewModel->OnHPChanged.AddUObject(this, &ThisClass::HandleHPChanged);
	ViewModel->OnPortraitChanged.AddUObject(this, &ThisClass::HandleImageChanged);
	ViewModel->OnIsMyTurnChanged.AddUObject(this, &ThisClass::HandleCharacterSelected);
	
	RefreshUI();
}

void UPBPartyMemberWidget::RefreshUI()
{
	if (!IsValid(MemberViewModel))
	{
		return;
	}
	
	HandleHPChanged(MemberViewModel->GetCharacterHPText());
	HandleImageChanged(MemberViewModel->GetPortrait());
	HandleCharacterSelected(MemberViewModel->bIsCharacterSelect());
}

void UPBPartyMemberWidget::NativeDestruct()
{
	if (IsValid(MemberViewModel))
	{
		MemberViewModel->OnHPChanged.RemoveAll(this);
		MemberViewModel->OnPortraitChanged.RemoveAll(this);
		MemberViewModel->OnIsMyTurnChanged.RemoveAll(this);
		MemberViewModel = nullptr;
	}
	
	Super::NativeDestruct();
}

void UPBPartyMemberWidget::HandleHPChanged(FText InCurrentHP)
{
	UpdataHPText();
}