// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDefaultCursorWidget.h"
#include "Components/TextBlock.h"

void UPBDefaultCursorWidget::SetDistance(float Meters)
{
	if (!IsValid(DistanceText))
	{
		return;
	}

	if (Meters <= 0.f)
	{
		ClearDistance();
		return;
	}

	DistanceText->SetText(FText::FromString(FString::Printf(TEXT("%.1fm"), Meters)));
	DistanceText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPBDefaultCursorWidget::ClearDistance()
{
	if (IsValid(DistanceText))
	{
		DistanceText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
