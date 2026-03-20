// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillNameFloatingWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UPBSkillNameFloatingWidget::SetSkillInfo(const FText& SkillName, EPBAbilityType AbilityType)
{
	if (IsValid(SkillNameText))
	{
		SkillNameText->SetText(SkillName);
	}
	
	// AbilityType에 따른 아이콘 설정 등 추가 확장 가능 
	// (예: Action, BonusAction 등에 따라 다른 아이콘 표시)
}

void UPBSkillNameFloatingWidget::PlayShowAnimation()
{
	if (CurrentAnim)
	{
		return;
	}

	if (IsValid(PopupAnim))
	{
		CurrentAnim = PopupAnim;
		PlayAnimation(PopupAnim);
	}
	else
	{
		OnAnimFinished.ExecuteIfBound();
	}
}

void UPBSkillNameFloatingWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	if (Animation == CurrentAnim)
	{
		OnAnimFinished.ExecuteIfBound();
	}
}
