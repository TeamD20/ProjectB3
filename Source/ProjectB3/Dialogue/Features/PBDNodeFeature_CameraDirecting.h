// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBDNodeFeature_Action.h"
#include "PBDNodeFeature_CameraDirecting.generated.h"

/**
 * 카메라 연출 Feature.
 * OnStartDialogueNode에서 지정 카메라로 전환한다.
 * TODO: 카메라 태그 기반 조회 시스템 연동 필요.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Camera Directing"))
class PROJECTB3_API UPBDNodeFeature_CameraDirecting : public UPBDNodeFeature_Action
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** 지정 카메라로 전환 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;

public:
    // 전환할 카메라를 식별하는 태그 (카메라 액터에 부여된 태그)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
    FGameplayTag CameraTag;

    // 카메라 전환 블렌드 시간 (초)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera")
    float BlendTime = 0.5f;
};
