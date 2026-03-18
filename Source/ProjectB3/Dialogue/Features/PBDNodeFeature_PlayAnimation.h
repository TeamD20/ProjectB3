// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBDNodeFeature_Action.h"
#include "PBDNodeFeature_PlayAnimation.generated.h"

class UAnimMontage;

/**
 * 캐릭터 애니메이션 재생 Feature.
 * OnStartDialogueNode에서 대상 Actor를 찾아 AnimMontage를 재생한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Play Animation"))
class PROJECTB3_API UPBDNodeFeature_PlayAnimation : public UPBDNodeFeature_Action
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** 대상 참여자를 찾아 AnimMontage를 재생 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;

public:
    // 애니메이션을 재생할 참여자 태그
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    FGameplayTag TargetParticipantTag;

    // 재생할 AnimMontage
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    TObjectPtr<UAnimMontage> AnimMontage;
};
