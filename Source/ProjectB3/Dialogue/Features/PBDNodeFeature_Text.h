// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogueFeatures/DNodeFeature_Text.h"
#include "PBDNodeFeature_Text.generated.h"

/**
 * 대사 텍스트 Feature.
 * OnStartDialogueNode에서 ViewModel->ShowText()를 호출한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Text"))
class PROJECTB3_API UPBDNodeFeature_Text : public UDNodeFeature_Text
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** ViewModel->ShowText()를 호출하여 대사 텍스트 표시를 구동 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;

public:
    // 표시할 대사 텍스트
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FText DialogueText;

protected:
    /*~ UDNodeFeature_Text Interface ~*/
    virtual FText GetDialogueText_Implementation() const override { return DialogueText; }
};
