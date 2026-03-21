// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBDNodeFeature_Action.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "PBDNodeFeature_CameraDirecting.generated.h"

class APBDialogueCameraActor;
class APlayerController;

/**
 * 카메라 연출 Feature.
 * OnStartDialogueNode에서 화자(Speaker) 참여자에 대한 카메라를 Manager에서 조회/생성하고
 * Over-the-Shoulder 구도로 ViewTarget을 전환한다.
 * 대화 종료 시 카메라 정리는 Manager가 담당한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Camera Directing"))
class PROJECTB3_API UPBDNodeFeature_CameraDirecting : public UPBDNodeFeature_Action
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** 화자 카메라 조회/생성 후 ViewTarget 전환 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;

private:
    /** Fade Out 완료 후 ViewTarget 전환 및 Fade In 시작 */
    void ApplyViewTargetAfterFade();
    
public:
    // 청자 참여자 태그 (없으면 화자 ForwardVector 역방향 사용)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
    FGameplayTag ListenerParticipantTag;

    // 카메라 배치 파라미터
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
    FPBDialogueCameraParams CameraParams;

private:
    // Fade 완료 후 ViewTarget 전환을 위한 타이머 핸들
    FTimerHandle FadeTimerHandle;

    // Fade 대기 중인 카메라 액터 (약참조)
    TWeakObjectPtr<APBDialogueCameraActor> PendingCamera;

    // Fade 처리 중인 PlayerController (약참조)
    TWeakObjectPtr<APlayerController> CachedPC;
};
