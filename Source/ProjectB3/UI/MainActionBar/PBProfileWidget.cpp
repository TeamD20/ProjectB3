// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/MainActionBar/PBProfileWidget.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewModel.h"
#include "Components/Image.h"
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
		
		HPPercentChangedHandle.Reset();
		HPTextChangedHandle.Reset();
		PortraitChangedHandle.Reset();
	}

	CurrentViewModel = InViewModel;

	// 새 뷰모델 연동 및 초기 뷰 설정
	if (CurrentViewModel)
	{
		HPPercentChangedHandle = CurrentViewModel->OnHPPercentValueChanged.AddUObject(this, &UPBProfileWidget::OnHPPercentChanged);
		HPTextChangedHandle = CurrentViewModel->OnHPChanged.AddUObject(this, &UPBProfileWidget::OnHPTextChanged);
		PortraitChangedHandle = CurrentViewModel->OnPortraitChanged.AddUObject(this, &UPBProfileWidget::OnPortraitImageChanged);

		OnHPPercentChanged(CurrentViewModel->GetHealthPercent());
		OnHPTextChanged(CurrentViewModel->GetCharacterHPText());
		OnPortraitImageChanged(CurrentViewModel->GetPortrait());
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
}

void UPBProfileWidget::OnHPTextChanged(FText InHPText)
{
	if (HPText)
	{
		HPText->SetText(InHPText);
	}
}

void UPBProfileWidget::OnPortraitImageChanged(TSoftObjectPtr<UTexture2D> NewPortrait)
{
	if (PortraitImage)
	{
		if (NewPortrait.IsValid() || !NewPortrait.IsNull())
		{
			PortraitImage->SetBrushFromSoftTexture(NewPortrait);
		}
		else
		{
			// 빈 텍스쳐나 기본 텍스쳐 처리 가능
			PortraitImage->SetBrushFromTexture(nullptr);
		}
	}
}
