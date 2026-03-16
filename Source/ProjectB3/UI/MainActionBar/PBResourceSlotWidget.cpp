// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBResourceSlotWidget.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UPBResourceSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void UPBResourceSlotWidget::SetIsActive(bool bIsActive)
{
	if (ActiveImage)
	{
		ActiveImage->SetVisibility(bIsActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UPBResourceSlotWidget::SetTextures(TSoftObjectPtr<UTexture2D> InBackgroundTexture, TSoftObjectPtr<UTexture2D> InActiveTexture)
{
	DefaultBackgroundTexture = InBackgroundTexture;
	DefaultActiveTexture = InActiveTexture;

	if (BackgroundImage && DefaultBackgroundTexture.IsValid())
	{
		BackgroundImage->SetBrushFromSoftTexture(DefaultBackgroundTexture);
	}

	if (ActiveImage && DefaultActiveTexture.IsValid())
	{
		ActiveImage->SetBrushFromSoftTexture(DefaultActiveTexture);
	}
}
