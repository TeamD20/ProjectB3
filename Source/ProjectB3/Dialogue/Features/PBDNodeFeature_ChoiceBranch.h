// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogueFeatures/DNodeFeature_ChoiceBranch.h"
#include "PBDNodeFeature_ChoiceBranch.generated.h"

/**
 * 선택지 분기 Feature.
 * OnStartDialogueNode에서 선택지 목록을 구성하여 ViewModel->ShowChoices()를 호출한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Choice Branch"))
class PROJECTB3_API UPBDNodeFeature_ChoiceBranch : public UDNodeFeature_ChoiceBranch
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** 선택지 목록을 구성하여 ViewModel->ShowChoices()를 호출 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;
};
