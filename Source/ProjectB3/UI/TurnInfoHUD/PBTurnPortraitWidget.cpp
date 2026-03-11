// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnPortraitWidget.h"
#include "PBTurnPortraitViewModel.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

void UPBTurnPortraitWidget::SetPortraitViewModel(UPBTurnPortraitViewModel* InViewModel)
{
	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.RemoveAll(this);
		PortraitViewModel->OnDeathStateChanged.RemoveAll(this);
		PortraitViewModel->OnDisplayNameChanged.RemoveAll(this);
		PortraitViewModel->OnPortraitChanged.RemoveAll(this);
		PortraitViewModel->OnHPPercentValueChanged.RemoveAll(this);
	}

	PortraitViewModel = InViewModel;

	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleCurrentTurnChanged);
		PortraitViewModel->OnDeathStateChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleDeathStateChanged);
		PortraitViewModel->OnDisplayNameChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleDisplayNameChanged);
		PortraitViewModel->OnPortraitChanged.AddUObject(this, &UPBTurnPortraitWidget::HandlePortraitChanged);
		PortraitViewModel->OnHPPercentValueChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleHPPercentChanged);

		// 초기 동기화
		HandleDisplayNameChanged(PortraitViewModel->GetDisplayName());
		HandlePortraitChanged(PortraitViewModel->GetPortrait());
		HandleCurrentTurnChanged(PortraitViewModel->IsCurrentTurn());
		HandleDeathStateChanged(PortraitViewModel->IsDead());
		HandleHPPercentChanged(PortraitViewModel->GetHealthPercent());
		
		bool bIsAlly = PortraitViewModel->IsAlly();
		BP_OnInitAllyState(bIsAlly);

		// 아군/적군 테두리 색상 처리
		if (OutlineImage)
		{
			// 블루프린트에서 설정한 아군/적군 테두리 색상 적용
			FLinearColor BorderColor = bIsAlly ? AllyBorderColor : EnemyBorderColor;
			OutlineImage->SetColorAndOpacity(BorderColor);
		}
	}
}

void UPBTurnPortraitWidget::NativeDestruct()
{
	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.RemoveAll(this);
		PortraitViewModel->OnDeathStateChanged.RemoveAll(this);
		PortraitViewModel->OnDisplayNameChanged.RemoveAll(this);
		PortraitViewModel->OnPortraitChanged.RemoveAll(this);
		PortraitViewModel->OnHPPercentValueChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

#include "ProjectB3/UI/Common/PBPortraitBaseWidget.h"

void UPBTurnPortraitWidget::HandleCurrentTurnChanged(bool bIsCurrentTurn)
{
	BP_OnCurrentTurnChanged(bIsCurrentTurn);
}

void UPBTurnPortraitWidget::HandleDeathStateChanged(bool bIsDead)
{
	BP_OnDeathStateChanged(bIsDead);
}

void UPBTurnPortraitWidget::HandleDisplayNameChanged(FText NewName)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetDisplayName(NewName);
	}
}

void UPBTurnPortraitWidget::HandlePortraitChanged(TSoftObjectPtr<UTexture2D> NewPortrait)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetPortraitImage(NewPortrait);
	}
}

void UPBTurnPortraitWidget::HandleHPPercentChanged(float InHealthPercent)
{
	if (DamageProgressBar)
	{
		DamageProgressBar->SetPercent(InHealthPercent);
	}
}
