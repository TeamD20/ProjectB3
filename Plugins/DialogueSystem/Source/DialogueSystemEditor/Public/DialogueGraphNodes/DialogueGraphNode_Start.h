// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DialogueGraphNode_Base.h"
#include "DialogueGraphNode_Start.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUESYSTEMEDITOR_API UDialogueGraphNode_Start : public UDialogueGraphNode_Base
{
	GENERATED_BODY()

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override {return FText::FromString("Start");}
	virtual FLinearColor GetNodeTitleColor() const override {return FLinearColor(FColor::Red);}
	virtual bool CanUserDeleteNode() const override {return false;}
	virtual void OnGraphNodeCreated() override;
	virtual FSlateColor GetPinColor() const override {return FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f));}
};