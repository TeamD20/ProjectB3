// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_ChoiceBranch.h"

#include "DialogueSystemTypes.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDNodeFeature_ChoiceBranch::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    UPBDialogueViewModel* ViewModel = InDialogueContext.GetContextObject<UPBDialogueViewModel>();
    if (!IsValid(ViewModel))
    {
        return;
    }

    // Choices 배열을 FPBDialogueChoiceInfo 배열로 변환
    TArray<FPBDialogueChoiceInfo> ChoiceInfos;
    ChoiceInfos.Reserve(Choices.Num());

    for (const FDialogueChoice& Choice : Choices)
    {
        FPBDialogueChoiceInfo Info;
        Info.ChoiceText = Choice.ChoiceText;
        Info.bAvailable = true;
        ChoiceInfos.Add(Info);
    }

    ViewModel->ShowChoices(ChoiceInfos);
}
