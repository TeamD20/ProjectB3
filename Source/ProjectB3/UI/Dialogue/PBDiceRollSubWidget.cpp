// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDiceRollSubWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDiceRollSubWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(RollButton))
    {
        RollButton->OnClicked.AddDynamic(this, &ThisClass::HandleRollButtonClicked);
    }
}

void UPBDiceRollSubWidget::InitializeDiceRoll(const FPBDiceRollDisplayInfo& InInfo)
{
    CurrentInfo = InInfo;

    if (IsValid(SkillNameText))
    {
        SkillNameText->SetText(InInfo.SkillName);
    }

    if (IsValid(DCText))
    {
        DCText->SetText(FText::AsNumber(InInfo.DC));
    }

    if (IsValid(ModifierText))
    {
        const FString ModStr = InInfo.Modifier >= 0
            ? FString::Printf(TEXT("+%d"), InInfo.Modifier)
            : FString::Printf(TEXT("%d"), InInfo.Modifier);
        ModifierText->SetText(FText::FromString(ModStr));
    }

    if (IsValid(ResultText))
    {
        ResultText->SetText(FText::GetEmpty());
    }

    if (IsValid(OutcomeText))
    {
        OutcomeText->SetText(FText::GetEmpty());
    }

    if (IsValid(RollButton))
    {
        RollButton->SetIsEnabled(true);
    }
}

void UPBDiceRollSubWidget::ShowDiceResult(const FPBDiceRollDisplayInfo& InResult)
{
    CurrentInfo = InResult;

    if (IsValid(ResultText))
    {
        ResultText->SetText(FText::AsNumber(InResult.TotalResult));
    }

    if (IsValid(OutcomeText))
    {
        if (InResult.bSuccess)
        {
            OutcomeText->SetText(FText::FromString(TEXT("성공!")));
            OutcomeText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
        }
        else
        {
            OutcomeText->SetText(FText::FromString(TEXT("실패")));
            OutcomeText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
        }
    }

    if (IsValid(RollButton))
    {
        RollButton->SetIsEnabled(false);
    }
}

void UPBDiceRollSubWidget::HandleRollButtonClicked()
{
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

    ViewModel->RequestDiceRoll();

    if (IsValid(RollButton))
    {
        RollButton->SetIsEnabled(false);
    }
}
