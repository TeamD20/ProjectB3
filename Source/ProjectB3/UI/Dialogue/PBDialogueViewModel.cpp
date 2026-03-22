// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDialogueViewModel.h"
#include "ProjectB3/Dialogue/PBDialogueManagerComponent.h"

void UPBDialogueViewModel::InitializeForPlayer(ULocalPlayer* InLocalPlayer)
{
    Super::InitializeForPlayer(InLocalPlayer);
    CachedChoices.Reset();
    CachedChoicePromptText = FText::GetEmpty();
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
    // "[화자이름] 대사 텍스트" 포맷으로 조합하여 저장
    const FText& SpeakerName = CachedSpeakerInfo.ParticipantName;
    CachedDialogueText = SpeakerName.IsEmpty()
        ? InText
        : FText::Format(NSLOCTEXT("Dialogue", "SpeakerTextFormat", "[{0}] {1}"), SpeakerName, InText);

    CachedChoices.Reset();
    OnTextChanged.Broadcast(CachedDialogueText);
}

void UPBDialogueViewModel::ShowChoices(const TArray<FPBDialogueChoiceInfo>& InChoices)
{
    // CachedDialogueText는 이미 "[화자이름] 텍스트" 포맷으로 조합된 상태
    CachedChoicePromptText = CachedDialogueText;
    CachedChoices = InChoices;
    CachedDialogueText = FText::GetEmpty();
    OnChoicesChanged.Broadcast(CachedChoices, CachedChoicePromptText);
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
