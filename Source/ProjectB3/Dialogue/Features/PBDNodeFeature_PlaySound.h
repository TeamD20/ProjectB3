// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBDNodeFeature_Action.h"
#include "PBDNodeFeature_PlaySound.generated.h"

class USoundBase;

/**
 * 사운드 재생 Feature.
 * OnStartDialogueNode에서 지정 사운드를 재생한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Play Sound"))
class PROJECTB3_API UPBDNodeFeature_PlaySound : public UPBDNodeFeature_Action
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** 지정 사운드를 재생 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;

public:
    // 재생할 사운드
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
    TObjectPtr<USoundBase> SoundCue;

    // 볼륨 배율
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
    float VolumeMultiplier = 1.0f;
};
