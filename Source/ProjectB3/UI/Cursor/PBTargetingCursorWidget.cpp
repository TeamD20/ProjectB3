// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTargetingCursorWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"

void UPBTargetingCursorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InitGaugeMaterial();
	ResetGauge();
}

void UPBTargetingCursorWidget::InitGaugeMaterial()
{
	if (!IsValid(GaugeMaterial) || !IsValid(GaugeImage))
	{
		return;
	}

	GaugeMID = UMaterialInstanceDynamic::Create(GaugeMaterial, this);
	if (IsValid(GaugeMID))
	{
		GaugeImage->SetBrushFromMaterial(GaugeMID);
	}
}

void UPBTargetingCursorWidget::SetGaugeProgress(float Progress)
{
	if (IsValid(GaugeMID))
	{
		GaugeMID->SetScalarParameterValue(TEXT("FillAmount"), FMath::Clamp(Progress, 0.f, 1.f));
	}

	if (IsValid(GaugeImage))
	{
		GaugeImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPBTargetingCursorWidget::SetGaugeCount(int32 Current, int32 Max)
{
	if (Max <= 0)
	{
		ResetGauge();
		return;
	}

	const float Progress = static_cast<float>(Current) / static_cast<float>(Max);
	SetGaugeProgress(Progress);

	if (IsValid(CountText))
	{
		CountText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Current, Max)));
		CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPBTargetingCursorWidget::ResetGauge()
{
	SetGaugeProgress(0.f);

	if (IsValid(GaugeImage))
	{
		GaugeImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (IsValid(CountText))
	{
		CountText->SetVisibility(ESlateVisibility::Collapsed);
	}
}
