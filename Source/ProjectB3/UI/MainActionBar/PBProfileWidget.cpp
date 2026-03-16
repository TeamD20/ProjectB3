// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/MainActionBar/PBProfileWidget.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewModel.h"
#include "ProjectB3/UI/Common/PBPortraitBaseWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UPBProfileWidget::InitializeProfile(UPBPartyMemberViewModel* InViewModel)
{
	if (CurrentViewModel == InViewModel)
	{
		return;
	}

	// 기존 바인딩 해제
	if (CurrentViewModel)
	{
		CurrentViewModel->OnHPPercentValueChanged.Remove(HPPercentChangedHandle);
		CurrentViewModel->OnHPChanged.Remove(HPTextChangedHandle);
		CurrentViewModel->OnPortraitChanged.Remove(PortraitChangedHandle);
		CurrentViewModel->OnNameChanged.Remove(NameChangedHandle);
		
		HPPercentChangedHandle.Reset();
		HPTextChangedHandle.Reset();
		PortraitChangedHandle.Reset();
		NameChangedHandle.Reset();
	}

	CurrentViewModel = InViewModel;

	// 새 뷰모델 연동 및 초기 뷰 설정
	if (CurrentViewModel)
	{
		HPPercentChangedHandle = CurrentViewModel->OnHPPercentValueChanged.AddUObject(this, &UPBProfileWidget::OnHPPercentChanged);
		HPTextChangedHandle = CurrentViewModel->OnHPChanged.AddUObject(this, &UPBProfileWidget::OnHPTextChanged);
		PortraitChangedHandle = CurrentViewModel->OnPortraitChanged.AddUObject(this, &UPBProfileWidget::OnPortraitImageChanged);
		NameChangedHandle = CurrentViewModel->OnNameChanged.AddUObject(this, &UPBProfileWidget::OnCharacterNameChanged);

		OnHPPercentChanged(CurrentViewModel->GetHealthPercent());
		OnHPTextChanged(CurrentViewModel->GetCharacterHPText());
		OnPortraitImageChanged(CurrentViewModel->GetPortrait());
		OnCharacterNameChanged(CurrentViewModel->GetCharacterName());
	}
}

void UPBProfileWidget::NativeDestruct()
{
	InitializeProfile(nullptr);
	Super::NativeDestruct();
}

void UPBProfileWidget::OnHPPercentChanged(float NewPercent)
{
	if (HPProgressBar)
	{
		HPProgressBar->SetPercent(NewPercent);
	}

	if (PortraitWidget)
	{
		PortraitWidget->SetHealthPercent(NewPercent);
	}
}

void UPBProfileWidget::OnHPTextChanged(FText InHPText)
{
	if (HPText)
	{
		HPText->SetText(InHPText);
	}

	if (PortraitWidget)
	{
		PortraitWidget->SetHealthText(InHPText);
	}
}

void UPBProfileWidget::OnPortraitImageChanged(TSoftObjectPtr<UTexture2D> NewPortrait)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetPortraitImage(NewPortrait);
	}
}

void UPBProfileWidget::OnCharacterNameChanged(FText NewName)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetDisplayName(NewName);
	}
}
