// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPartyMemberTooltipRowWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPBPartyMemberTooltipRowWidget::InitializeRowData(TSoftObjectPtr<UTexture2D> InIcon, const FText& InText)
{
	if (RowIcon)
	{
		if (!InIcon.IsNull())
		{
			RowIcon->SetBrushFromSoftTexture(InIcon);
			RowIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			RowIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (RowText)
	{
		RowText->SetText(InText);
	}
}
