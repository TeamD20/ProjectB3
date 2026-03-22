// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_CameraDirecting.h"

#include "Camera/PlayerCameraManager.h"
#include "DialogueNode.h"
#include "DialogueSystemTypes.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/Camera/PBDialogueCameraActor.h"
#include "ProjectB3/Dialogue/PBDialogueManagerComponent.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDNodeFeature_CameraDirecting::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    UPBDialogueViewModel* ViewModel = InDialogueContext.GetContextObject<UPBDialogueViewModel>();
    if (!IsValid(ViewModel))
    {
        return;
    }

    UPBDialogueManagerComponent* Manager = ViewModel->GetDialogueManager();
    if (!IsValid(Manager))
    {
        return;
    }

    APlayerController* PC = ViewModel->GetOwningPlayerController();
    if (!IsValid(PC))
    {
        return;
    }

    // 노드의 ParticipantTag를 화자 태그로 사용
    const FGameplayTag SpeakerTag = InDialogueNode->ParticipantTag;

    // 화자/청자 Actor 조회
    AActor* SpeakerActor = FindParticipantActor(InDialogueContext, SpeakerTag);
    AActor* ListenerActor = ListenerParticipantTag.IsValid()
        ? FindParticipantActor(InDialogueContext, ListenerParticipantTag)
        : nullptr;

    if (!IsValid(SpeakerActor))
    {
        return;
    }

    // Manager에서 화자 전용 카메라 조회 또는 생성
    APBDialogueCameraActor* Camera = Manager->GetOrCreateCamera(SpeakerTag);
    if (!IsValid(Camera))
    {
        return;
    }

    // 추적 대상 및 파라미터 갱신 (ViewTarget 여부와 무관하게 항상 수행)
    Camera->UpdateTracking(SpeakerActor, ListenerActor, CameraParams);

    // 이미 동일한 카메라가 ViewTarget이면 전환 불필요
    if (PC->GetViewTarget() == Camera)
    {
        return;
    }

    // 진행 중인 Fade 타이머가 있으면 정리
    if (FadeTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(FadeTimerHandle);
    }

    CachedPC = PC;
    PendingCamera = Camera;

    // FadeOutDuration이 0이면 즉시 전환
    if (CameraParams.FadeOutDuration <= 0.f)
    {
        ApplyViewTargetAfterFade();
        return;
    }

    // Fade Out 시작 후 완료 시점에 ViewTarget 전환
    PC->PlayerCameraManager->StartCameraFade(
        0.f, 1.f,
        CameraParams.FadeOutDuration,
        CameraParams.FadeColor,
        false,
        true   // bHoldWhenFinished: 전환 전까지 검은 화면 유지
    );

    GetWorld()->GetTimerManager().SetTimer(
        FadeTimerHandle,
        this,
        &UPBDNodeFeature_CameraDirecting::ApplyViewTargetAfterFade,
        CameraParams.FadeOutDuration,
        false
    );
}

void UPBDNodeFeature_CameraDirecting::ApplyViewTargetAfterFade()
{
    if (!CachedPC.IsValid() || !PendingCamera.IsValid())
    {
        return;
    }

    // 즉시 컷 전환
    CachedPC->SetViewTargetWithBlend(PendingCamera.Get(), CameraParams.BlendTime);

    // Fade In
    if (CameraParams.FadeInDuration > 0.f)
    {
        CachedPC->PlayerCameraManager->StartCameraFade(
            1.f, 0.f,
            CameraParams.FadeInDuration,
            CameraParams.FadeColor,
            false,
            false
        );
    }

    CachedPC.Reset();
    PendingCamera.Reset();
}
