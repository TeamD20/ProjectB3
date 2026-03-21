// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_Text.h"

#include "DialogueNode.h"
#include "DialogueSystemTypes.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDNodeFeature_Text::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    UPBDialogueViewModel* ViewModel = InDialogueContext.GetContextObject<UPBDialogueViewModel>();
    if (!IsValid(ViewModel))
    {
        return;
    }

    ViewModel->SetSpeakerInfo(BuildSpeakerInfo(InDialogueNode, InDialogueContext));
    ViewModel->ShowText(GetDialogueText());
}

FPBDialogueParticipantDisplayInfo UPBDNodeFeature_Text::BuildSpeakerInfo(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    FPBDialogueParticipantDisplayInfo Info;
    if (!IsValid(InDialogueNode))
    {
        return Info;
    }
    
    if (APBCharacterBase* PBCharacter = Cast<APBCharacterBase>(InDialogueContext.FindParticipantActor(InDialogueNode->ParticipantTag)))
    {
        Info.ParticipantName = PBCharacter->GetCharacterIdentity().DisplayName;
    }
    
    return Info;
}