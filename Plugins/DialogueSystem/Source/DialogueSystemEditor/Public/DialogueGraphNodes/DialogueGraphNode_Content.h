// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DialogueGraphNode_Base.h"
#include "DialogueGraphNode_Content.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUESYSTEMEDITOR_API UDialogueGraphNode_Content : public UDialogueGraphNode_Base
{
	GENERATED_BODY()

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override;
	virtual void OnGraphNodeCreated() override;
	virtual FSlateColor GetPinColor() const override {return FSlateColor(FLinearColor(1.0f, 0.5f, 0.0f));}
};
