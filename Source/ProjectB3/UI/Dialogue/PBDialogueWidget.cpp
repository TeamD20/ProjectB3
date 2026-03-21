// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDialogueWidget.h"

#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Components/Overlay.h"
#include "ProjectB3/UI/Dialogue/PBDialogueChoiceEntryWidget.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"
#include "ProjectB3/UI/Dialogue/PBDiceRollSubWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"

void UPBDialogueWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetIsFocusable(true);

    UPBViewModelSubsystem* VMSubsystem = UPBUIBlueprintLibrary::GetViewModelSubsystem(GetOwningPlayer());
    if (!IsValid(VMSubsystem))
    {
        return;
    }

    UPBDialogueViewModel* ViewModel = VMSubsystem->GetGlobalViewModel<UPBDialogueViewModel>();
    if (!IsValid(ViewModel))
    {
        return;
    }

    BoundViewModel = ViewModel;

    ViewModel->OnTextChanged.AddUObject(this, &ThisClass::HandleTextChanged);
    ViewModel->OnChoicesChanged.AddUObject(this, &ThisClass::HandleChoicesChanged);
    ViewModel->OnDiceRollChanged.AddUObject(this, &ThisClass::HandleDiceRollChanged);
    ViewModel->OnDiceResultChanged.AddUObject(this, &ThisClass::HandleDiceResultChanged);
}

void UPBDialogueWidget::NativeDestruct()
{
    if (BoundViewModel.IsValid())
    {
        BoundViewModel->OnTextChanged.RemoveAll(this);
        BoundViewModel->OnChoicesChanged.RemoveAll(this);
        BoundViewModel->OnDiceRollChanged.RemoveAll(this);
        BoundViewModel->OnDiceResultChanged.RemoveAll(this);
        BoundViewModel.Reset();
    }

    Super::NativeDestruct();
}

void UPBDialogueWidget::RequestContinueByButtonClick()
{
    RequestContinueInternal();
}

FReply UPBDialogueWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CanRequestContinueFromInput())
    {
        RequestContinueInternal();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UPBDialogueWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (CanRequestContinueFromInput() && IsContinueKey(InKeyEvent.GetKey()))
    {
        RequestContinueInternal();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPBDialogueWidget::HandleTextChanged(const FText& InText)
{
    if (IsValid(DialogueTextBlock))
    {
        DialogueTextBlock->SetText(InText);
        DialogueTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    if (IsValid(ChoiceOverlay))
    {
        ChoiceOverlay->SetVisibility(ESlateVisibility::Collapsed);
    }
    
    if (IsValid(ChoicePanel))
    {
        ChoicePanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(DiceRollPanel))
    {
        DiceRollPanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(ContinuePrompt))
    {
        ContinuePrompt->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    SetKeyboardFocus();
}

void UPBDialogueWidget::HandleChoicesChanged(const TArray<FPBDialogueChoiceInfo>& InChoices, const FText& InPromptText)
{
    if (IsValid(DialogueTextBlock))
    {
        DialogueTextBlock->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 이전 노드 대사 텍스트가 있으면 "[화자이름] 대사" 포맷으로 선택지 상단에 표시
    if (IsValid(ChoicePromptTextBlock))
    {
        if (InPromptText.IsEmpty())
        {
            ChoicePromptTextBlock->SetVisibility(ESlateVisibility::Collapsed);
        }
        else
        {
            ChoicePromptTextBlock->SetText(InPromptText);
            ChoicePromptTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }

    if (IsValid(DiceRollPanel))
    {
        DiceRollPanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(ContinuePrompt))
    {
        ContinuePrompt->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (!IsValid(ChoicePanel))
    {
        return;
    }
    
    if (IsValid(ChoiceOverlay))
    {
        ChoiceOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    ChoicePanel->ClearChildren();
    ChoicePanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (!ChoiceEntryWidgetClass)
    {
        return;
    }

    if (!BoundViewModel.IsValid())
    {
        return;
    }

    for (int32 ChoiceIndex = 0; ChoiceIndex < InChoices.Num(); ++ChoiceIndex)
    {
        UPBDialogueChoiceEntryWidget* EntryWidget = CreateWidget<UPBDialogueChoiceEntryWidget>(this, ChoiceEntryWidgetClass);
        if (!IsValid(EntryWidget))
        {
            continue;
        }

        EntryWidget->InitializeChoice(InChoices[ChoiceIndex], ChoiceIndex, BoundViewModel.Get());
        ChoicePanel->AddChild(EntryWidget);
    }
}

void UPBDialogueWidget::HandleDiceRollChanged(const FPBDiceRollDisplayInfo& InInfo)
{
    if (IsValid(DialogueTextBlock))
    {
        DialogueTextBlock->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(ChoiceOverlay))
    {
        ChoiceOverlay->SetVisibility(ESlateVisibility::Collapsed);
    }
    
    if (IsValid(ChoicePanel))
    {
        ChoicePanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(ContinuePrompt))
    {
        ContinuePrompt->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (IsValid(DiceRollPanel))
    {
        DiceRollPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        DiceRollPanel->InitializeDiceRoll(InInfo);
    }
}

void UPBDialogueWidget::HandleDiceResultChanged(const FPBDiceRollDisplayInfo& InResult)
{
    if (IsValid(DiceRollPanel))
    {
        DiceRollPanel->ShowDiceResult(InResult);
    }
}

bool UPBDialogueWidget::CanRequestContinueFromInput() const
{
    if (!BoundViewModel.IsValid())
    {
        return false;
    }

    if (!IsValid(DialogueTextBlock))
    {
        return false;
    }

    return DialogueTextBlock->GetVisibility() != ESlateVisibility::Collapsed
        && DialogueTextBlock->GetVisibility() != ESlateVisibility::Hidden;
}

void UPBDialogueWidget::RequestContinueInternal()
{
    if (!CanRequestContinueFromInput())
    {
        return;
    }

    BoundViewModel->RequestContinue();
}

bool UPBDialogueWidget::IsContinueKey(const FKey& InKey) const
{
    return InKey == EKeys::Enter
        || InKey == EKeys::Virtual_Accept
        || InKey == EKeys::SpaceBar
        || InKey == EKeys::E
        || InKey == EKeys::F;
}
