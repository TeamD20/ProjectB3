// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCombatStateTextWidget.h"
#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"

void UPBCombatStateTextWidget::SetCombatState(bool bIsStarting)
{
	if (!IsValid(StateText))
	{
		return;
	}

	if (bIsStarting)
	{
		StateText->SetText(FText::FromString(TEXT("전투 돌입")));
		CurrentAnim = EntryAnim;
	}
	else
	{
		StateText->SetText(FText::FromString(TEXT("전투 종료")));
		CurrentAnim = ExitAnim;
	}

	if (CurrentAnim)
	{
		PlayAnimation(CurrentAnim);
	}
	else
	{
		// 애니메이션이 없으면 즉시 종료 콜백 호출
		OnAnimFinished.ExecuteIfBound();
	}
}

void UPBCombatStateTextWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	if (Animation == CurrentAnim)
	{
		OnAnimFinished.ExecuteIfBound();
	}
}
