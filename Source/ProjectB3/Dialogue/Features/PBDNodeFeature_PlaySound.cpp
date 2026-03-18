// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_PlaySound.h"

#include "DialogueSystemTypes.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDNodeFeature_PlaySound::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    if (!IsValid(SoundCue))
    {
        return;
    }

    UPBDialogueViewModel* ViewModel = InDialogueContext.GetContextObject<UPBDialogueViewModel>();
    if (!IsValid(ViewModel))
    {
        return;
    }

    APlayerController* PC = ViewModel->GetOwningPlayerController();
    if (!IsValid(PC) || !IsValid(PC->GetWorld()))
    {
        return;
    }

    UGameplayStatics::PlaySound2D(PC->GetWorld(), SoundCue, VolumeMultiplier);
}
