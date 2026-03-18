// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_CameraDirecting.h"

#include "DialogueSystemTypes.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDNodeFeature_CameraDirecting::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    UPBDialogueViewModel* ViewModel = InDialogueContext.GetContextObject<UPBDialogueViewModel>();
    if (!IsValid(ViewModel))
    {
        return;
    }

    APlayerController* PC = ViewModel->GetOwningPlayerController();
    if (!IsValid(PC))
    {
        return;
    }

    // TODO: CameraTag 기반으로 씬에서 카메라 액터를 찾아 전환하는 시스템 연동
    // 현재는 플레이스홀더
}
