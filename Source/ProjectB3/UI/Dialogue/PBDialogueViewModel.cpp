// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDialogueViewModel.h"
#include "ProjectB3/Dialogue/PBDialogueManagerComponent.h"

void UPBDialogueViewModel::InitializeForPlayer(ULocalPlayer* InLocalPlayer)
{
    Super::InitializeForPlayer(InLocalPlayer);
    CachedChoices.Reset();
    CachedDialogueText = FText::GetEmpty();
    CachedDiceRollInfo = FPBDiceRollDisplayInfo();
    CachedSpeakerInfo = FPBDialogueParticipantDisplayInfo();
}

void UPBDialogueViewModel::Deinitialize()
{
    DialogueManager.Reset();
    Super::Deinitialize();
}

void UPBDialogueViewModel::SetDialogueManager(UPBDialogueManagerComponent* InManager)
{
    DialogueManager = InManager;
}

void UPBDialogueViewModel::SetSpeakerInfo(const FPBDialogueParticipantDisplayInfo& InInfo)
{
    CachedSpeakerInfo = InInfo;
    OnSpeakerChanged.Broadcast(CachedSpeakerInfo);
}

void UPBDialogueViewModel::ShowText(const FText& InText)
{
    CachedDialogueText = InText;
    CachedChoices.Reset();
    OnTextChanged.Broadcast(CachedDialogueText);
}

void UPBDialogueViewModel::ShowChoices(const TArray<FPBDialogueChoiceInfo>& InChoices)
{
    CachedChoices = InChoices;
    CachedDialogueText = FText::GetEmpty();
    OnChoicesChanged.Broadcast(CachedChoices);
}

void UPBDialogueViewModel::ShowDiceRoll(const FPBDiceRollDisplayInfo& InInfo)
{
    CachedDiceRollInfo = InInfo;
    OnDiceRollChanged.Broadcast(CachedDiceRollInfo);
}

void UPBDialogueViewModel::ShowDiceResult(const FPBDiceRollDisplayInfo& InResult)
{
    CachedDiceRollInfo = InResult;
    OnDiceResultChanged.Broadcast(CachedDiceRollInfo);
}

void UPBDialogueViewModel::RequestContinue()
{
    if (UPBDialogueManagerComponent* Manager = DialogueManager.Get())
    {
        Manager->ProgressDialogue(0);
    }
}

void UPBDialogueViewModel::SelectChoice(int32 Index)
{
    if (UPBDialogueManagerComponent* Manager = DialogueManager.Get())
    {
        Manager->ProgressDialogue(Index);
    }
}

void UPBDialogueViewModel::RequestDiceRoll()
{
    if (UPBDialogueManagerComponent* Manager = DialogueManager.Get())
    {
        Manager->RequestDiceRoll();
    }
}
