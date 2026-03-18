// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DialogueGraphNode_Base.h"
#include "DialogueGraphNode_End.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUESYSTEMEDITOR_API UDialogueGraphNode_End : public UDialogueGraphNode_Base
{
	GENERATED_BODY()
	
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override {return FText::FromString("End");}
	virtual FLinearColor GetNodeTitleColor() const override {return FLinearColor(FColor::Green);}
	virtual bool CanUserDeleteNode() const override {return true;}
	virtual void OnGraphNodeCreated() override;
	virtual FSlateColor GetPinColor() const override {return FSlateColor(FLinearColor(0.2f, 1.0f, 0.2f));}
};
