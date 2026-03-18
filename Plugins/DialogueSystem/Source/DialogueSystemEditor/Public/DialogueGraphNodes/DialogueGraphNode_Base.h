// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "EdGraph/EdGraphNode.h"
#include "DialogueGraphNode_Base.generated.h"

class UDialogueData;
class UDialogueNode;
/**
 * 
 */
UCLASS()
class DIALOGUESYSTEMEDITOR_API UDialogueGraphNode_Base : public UEdGraphNode
{
	GENERATED_BODY()

public:
	/*~ UEdGraphNode Interface ~*/
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void OnGraphNodeCreated();
	virtual void OnDialogueNodePropertiesChanged();
	virtual void SyncPinsWithBranch();
	virtual UEdGraphPin* CreateDialoguePin(EEdGraphPinDirection Direction, FName Name);
	virtual FName GetPinSubCategory() {return FName(TEXT("DialoguePin"));}
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	
	/*~ UDialogueGraphNode_Base Interface ~*/
	UDialogueData* GetDialogueData() const;
	UDialogueNode* InitDialogueNode(const UDialogueNode* InPresetNode);

	virtual FSlateColor GetPinColor() const { return FSlateColor( FLinearColor::White);	}
	virtual FSlateColor GetPinTextColor() const { return FSlateColor(PinTextColor);	}

public:
	UPROPERTY()
	UDialogueNode* DialogueNode;
	
private:
	FLinearColor PinTextColor = FLinearColor::White;
};