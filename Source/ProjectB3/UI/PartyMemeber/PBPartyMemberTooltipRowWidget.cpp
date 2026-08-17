// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 강리한 (파티 멤버 툴팁 항목 표시 UI)

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
