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
        ResultText->SetVisibility(ESlateVisibility::Hidden);
    }

    if (IsValid(OutcomeText))
    {
        OutcomeText->SetText(FText::GetEmpty());
        OutcomeText->SetVisibility(ESlateVisibility::Hidden);
    }

    if (IsValid(RollButton))
    {
        RollButton->SetIsEnabled(true);
    }

    bResultShown = false;
}

void UPBDiceRollSubWidget::ShowDiceResult(const FPBDiceRollDisplayInfo& InResult)
{
    CurrentInfo = InResult;

    if (IsValid(ResultText))
    {
        ResultText->SetText(FText::AsNumber(InResult.TotalResult));
        ResultText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    if (IsValid(RollButton))
    {
        RollButton->SetIsEnabled(false);
    }

    // 성공/실패 텍스트 및 색상은 BP 애니메이션에서 처리
    if (InResult.bSuccess)
    {
        PlaySuccessAnimation(InResult);
    }
    else
    {
        PlayFailureAnimation(InResult);
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
