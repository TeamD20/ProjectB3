// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPortraitBaseWidget.h"
#include "Components/ProgressBar.h"

void UPBPortraitBaseWidget::SetPortraitImage(TSoftObjectPtr<UTexture2D> InPortrait)
{
	if (PortraitImage)
	{
		PortraitImage->SetBrushFromSoftTexture(InPortrait);
	}

	BP_OnPortraitChanged(InPortrait);
}

void UPBPortraitBaseWidget::SetDisplayName(const FText& InName)
{
	if (DisplayNameText)
	{
		DisplayNameText->SetText(InName);
	}

	BP_OnDisplayNameChanged(InName);
}

void UPBPortraitBaseWidget::SetHealthPercent(float InPercent)
{
	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(InPercent);
	}

	BP_OnHealthPercentChanged(InPercent);
}

void UPBPortraitBaseWidget::SetHealthText(const FText& InText)
{
	if (HealthText)
	{
		HealthText->SetText(InText);
	}

	BP_OnHealthTextChanged(InText);
}
