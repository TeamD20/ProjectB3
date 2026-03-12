// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/SkillBar/PBSkillIconWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void UPBSkillIconWidget::UpdateSlot(const FPBSkillSlotData& InSlotData)
{
	// 1. 아이콘 설정
	if (SkillIcon && InSlotData.Icon.IsValid())
	{
		SkillIcon->SetBrushFromTexture(InSlotData.Icon.Get());
	}

	// 2. 포커스 테두리 가시성 설정 (bIsActive 변수 기반)
	if (FocusBorder)
	{
		FocusBorder->SetVisibility(InSlotData.bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 3. 발동 가능 여부 시뮬레이션 (DisabledOverlay)
	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(InSlotData.bCanActivate ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	// 4. 쿨다운 표시
	if (CooldownText)
	{
		if (InSlotData.CooldownRemaining > 0)
		{
			CooldownText->SetVisibility(ESlateVisibility::Visible);
			CooldownText->SetText(FText::AsNumber(InSlotData.CooldownRemaining));
		}
		else
		{
			CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
