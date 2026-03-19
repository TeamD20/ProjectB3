// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_PlayAnimation.h"

#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "DialogueSystemTypes.h"

void UPBDNodeFeature_PlayAnimation::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    if (!IsValid(AnimMontage))
    {
        return;
    }

    AActor* TargetActor = FindParticipantActor(InDialogueContext, TargetParticipantTag);
    if (!IsValid(TargetActor))
    {
        return;
    }

    USkeletalMeshComponent* Mesh = TargetActor->FindComponentByClass<USkeletalMeshComponent>();
    if (!IsValid(Mesh))
    {
        return;
    }

    UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
    if (!IsValid(AnimInstance))
    {
        return;
    }

    AnimInstance->Montage_Play(AnimMontage);
}
