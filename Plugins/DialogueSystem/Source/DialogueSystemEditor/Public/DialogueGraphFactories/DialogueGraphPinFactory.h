// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "EdGraphUtilities.h"
#include "SGraphPin.h"
#include "DialogueGraphNodes/DialogueGraphNode_Base.h"

class UEdGraphNode;

class DIALOGUESYSTEMEDITOR_API SDialogueGraphPin : public SGraphPin
{
public:
SLATE_BEGIN_ARGS(SDialogueGraphPin)
	{}
SLATE_END_ARGS()

void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		if (UEdGraphNode* GraphNode = InGraphPinObj->GetOwningNode())
		{
			if (UDialogueGraphNode_Base* DNode = Cast<UDialogueGraphNode_Base>(GraphNode))
			{
				OwningNode = DNode;
			}
		}
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	}

protected:
	virtual FSlateColor GetPinColor() const override
	{
		if (OwningNode.IsValid())
		{
			return OwningNode->GetPinColor();
		}
		return FSlateColor(FLinearColor::Gray);
	}

	virtual FSlateColor GetPinTextColor() const override
	{
		if (OwningNode.IsValid())
		{
			return OwningNode->GetPinTextColor();
		}
		return FSlateColor(FLinearColor::White);
	}

private:
	TWeakObjectPtr<UDialogueGraphNode_Base> OwningNode;
};

/** PinFactory */
struct DIALOGUESYSTEMEDITOR_API FDialogueGraphPinFactory : public FGraphPanelPinFactory
{
public:
	virtual ~FDialogueGraphPinFactory() override {}

	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* Pin) const override
	{
		if ( Pin->PinType.PinSubCategory == FName(TEXT("DialoguePin")) )
		{
			return SNew(SDialogueGraphPin, Pin);	
		}
		
		return nullptr;
	}
};