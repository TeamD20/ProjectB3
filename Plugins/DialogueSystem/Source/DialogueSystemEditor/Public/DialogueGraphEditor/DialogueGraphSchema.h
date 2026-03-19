// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "DialogueNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "DialogueGraphSchema.generated.h"

class UDialogueDataEditorConfig;
class UDialogueNode;

USTRUCT()
struct FNewNodeAction : public FEdGraphSchemaAction {
	GENERATED_BODY()

public:
	FNewNodeAction() {}
	FNewNodeAction(UClass* InClassType, UDialogueNode* InPreset, const FText& InNodeCategory, const FText& InMenuDesc, const FText& InToolTip, const int32 InGrouping)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InToolTip, InGrouping), GraphNodeType(InClassType), PresetNode(InPreset) {}

	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2f& Location, bool bSelectNewNode = true) override;

protected:
	UClass* GraphNodeType = nullptr;
	UPROPERTY()
	UDialogueNode* PresetNode = nullptr;
};
/**
 * 
 */
UCLASS()
class DIALOGUESYSTEMEDITOR_API UDialogueGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;

};