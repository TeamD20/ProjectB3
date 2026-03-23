// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBFloatingTextWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/Payload/PBFloatingTextPayload.h"

void UPBFloatingTextWidget::SetPayload(UPBFloatingTextPayload* Payload)
{
	if (!IsValid(Payload))
	{
		return;
	}

	// 수치 표시 (0이 아닌 경우)
	if (!FMath::IsNearlyZero(Payload->Magnitude))
	{
		const FString MagnitudeStr = (Payload->Magnitude > 0.f)
			? FString::Printf(TEXT("+%d"), FMath::RoundToInt(Payload->Magnitude))
			: FString::Printf(TEXT("%d"), FMath::RoundToInt(Payload->Magnitude));
		MagnitudeText->SetText(FText::FromString(MagnitudeStr));
		MagnitudeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		MagnitudeText->SetVisibility(ESlateVisibility::Collapsed);
	}

	bIsCritical = Payload->MetaTag.MatchesTagExact(PBGameplayTags::Combat_Hit_Critical);

	// 라벨 텍스트 표시 (비어있지 않은 경우)
	if (!Payload->Text.IsEmpty())
	{
		LabelText->SetText(Payload->Text);
		LabelText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		LabelText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 아이콘 처리
	if (IconImage)
	{
		if (!Payload->Icon.IsNull())
		{
			IconImage->SetBrushFromSoftTexture(Payload->Icon);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPBFloatingTextWidget::PlayShowAnimation()
{
	if (CurrentAnimation)
	{
		return;
	}

	// 크리티컬이면 CriticalAnim, 아니면 DefaultAnim 재생
	// CriticalAnim이 설정되지 않은 경우 DefaultAnim으로 폴백
	UWidgetAnimation* AnimToPlay = (bIsCritical && IsValid(CriticalAnim))
		? CriticalAnim
		: DefaultAnim;

	if (IsValid(AnimToPlay))
	{
		CurrentAnimation = AnimToPlay;
		PlayAnimation(AnimToPlay);
	}
	else
	{
		// 애니메이션이 없으면 즉시 종료 알림
		OnFloatingTextAnimationFinished.ExecuteIfBound();
	}
}

void UPBFloatingTextWidget::OnAnimationFinished_Implementation(const UWidgetAnimation* Animation)
{
	if (Animation == CurrentAnimation)
	{
		OnFloatingTextAnimationFinished.ExecuteIfBound();
	}
}
