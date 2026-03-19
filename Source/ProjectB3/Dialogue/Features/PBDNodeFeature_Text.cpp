// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_Text.h"

#include "DialogueSystemTypes.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDNodeFeature_Text::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    UPBDialogueViewModel* ViewModel = InDialogueContext.GetContextObject<UPBDialogueViewModel>();
    if (!IsValid(ViewModel))
    {
        return;
    }

    ViewModel->ShowText(GetDialogueText());
}
