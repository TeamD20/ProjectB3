// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDialogueChoiceEntryWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/RichTextBlock.h"
#include "InputCoreTypes.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDialogueChoiceEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(ChoiceButton))
    {
        ChoiceButton->OnClicked.AddDynamic(this, &ThisClass::HandleChoiceButtonClicked);
    }

    // 인디케이터 초기 상태: 숨김
    if (IsValid(IndicatorImage))
    {
        IndicatorImage->SetVisibility(ESlateVisibility::Hidden);
    }

    // 텍스트 초기 색상 설정
    if (IsValid(ChoiceText))
    {
        ChoiceText->SetDefaultColorAndOpacity(FSlateColor(DefaultTextColor));
    }
}

void UPBDialogueChoiceEntryWidget::NativeDestruct()
{
    if (IsValid(ChoiceButton))
    {
        ChoiceButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleChoiceButtonClicked);
    }

    BoundViewModel.Reset();
    Super::NativeDestruct();
}

FReply UPBDialogueChoiceEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!IsValid(ChoiceButton)
        && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
        && bChoiceAvailable)
    {
        RequestSelectChoice();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UPBDialogueChoiceEntryWidget::InitializeChoice(const FPBDialogueChoiceInfo& InInfo, int32 InChoiceIndex, UPBDialogueViewModel* InViewModel)
{
    ChoiceIndex = InChoiceIndex;
    bChoiceAvailable = InInfo.bAvailable;
    BoundViewModel = InViewModel;

    if (IsValid(ChoiceText))
    {
        ChoiceText->SetText(InInfo.ChoiceText);
    }

    if (IsValid(ChoiceButton))
    {
        ChoiceButton->SetIsEnabled(InInfo.bAvailable);
    }
    else
    {
        SetIsEnabled(InInfo.bAvailable);
    }

    // 비활성 사유 표시
    if (IsValid(UnavailableReasonText))
    {
        if (!InInfo.bAvailable && !InInfo.UnavailableReason.IsEmpty())
        {
            UnavailableReasonText->SetText(InInfo.UnavailableReason);
            UnavailableReasonText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            UnavailableReasonText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UPBDialogueChoiceEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (IsValid(ChoiceText))
    {
        ChoiceText->SetDefaultColorAndOpacity(FSlateColor(HoveredTextColor));
    }

    if (IsValid(IndicatorImage))
    {
        IndicatorImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }
}

void UPBDialogueChoiceEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (IsValid(ChoiceText))
    {
        ChoiceText->SetDefaultColorAndOpacity(FSlateColor(DefaultTextColor));
    }

    if (IsValid(IndicatorImage))
    {
        IndicatorImage->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UPBDialogueChoiceEntryWidget::HandleChoiceButtonClicked()
{
    RequestSelectChoice();
}

void UPBDialogueChoiceEntryWidget::RequestSelectChoice()
{
    if (!bChoiceAvailable)
    {
        return;
    }

    if (BoundViewModel.IsValid())
    {
        BoundViewModel->SelectChoice(ChoiceIndex);
    }
}
