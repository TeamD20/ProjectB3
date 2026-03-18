// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphEditor/DialogueGraphSchema.h"
#include "DialogueSystemEditor.h"
#include "DialogueGraphEditor/DialogueDataEditorConfig.h"
#include "DialogueGraphNodes/DialogueGraphNode_End.h"
#include "DialogueGraphNodes/DialogueGraphNode_Content.h"
#include "DialogueGraphNodes/DialogueGraphNode_Base.h"
#include "DialogueGraphNodes/DialogueGraphNode_Start.h"

UEdGraphNode* FNewNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin,
	const FVector2f& Location, bool bSelectNewNode)
{
	UDialogueGraphNode_Base* Result = NewObject<UDialogueGraphNode_Base>(ParentGraph, GraphNodeType);
	Result->CreateNewGuid();
	Result->NodePosX = Location.X;
	Result->NodePosY = Location.Y;
	Result->InitDialogueNode(PresetNode);
	Result->OnGraphNodeCreated();
	Result->SyncPinsWithBranch();
	
	if (FromPin != nullptr)
	{
		if (UEdGraphPin* InputPin = Result->GetPinWithDirectionAt(0,EGPD_Input))
		{
			Result->GetSchema()->TryCreateConnection(FromPin, InputPin);
		}
	}

	ParentGraph->Modify();
	ParentGraph->AddNode(Result, true, true);

	return Result;
}

void UDialogueGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UDialogueDataEditorConfig* Config = FDialogueSystemEditorModule::GetDialogueDataEditorConfig();
	check(Config);
	
	TSharedPtr<FNewNodeAction> NewEmptyNodeAction(
		new FNewNodeAction(
			UDialogueGraphNode_Content::StaticClass(),
			Config->GetEmptyNodePreset(),
			FText::FromString(TEXT("Nodes")),
			FText::FromString(TEXT("Create Empty Node")),
			FText::FromString(TEXT("Makes a new empty dialogue node")),
			0
		)
	);

	TSharedPtr<FNewNodeAction> NewEndNodeAction(
		new FNewNodeAction(
			UDialogueGraphNode_End::StaticClass(),
			Config->GetEmptyNodePreset(),
			FText::FromString(TEXT("Nodes")),
			FText::FromString(TEXT("Create End Node")),
			FText::FromString(TEXT("Makes a new dialogue end node")),
			0
		)
	);
	
	ContextMenuBuilder.AddAction(NewEmptyNodeAction);
	ContextMenuBuilder.AddAction(NewEndNodeAction);

	// Config의 Preset으로 부터 컨텍스트 메뉴 추가
	for (FDialogueNodePreset& Preset : Config->Presets)
	{
		if (Preset.PresetNode == nullptr){ continue; }
		
		TSharedPtr<FNewNodeAction> NewNodeAction(
			new FNewNodeAction(
				UDialogueGraphNode_Content::StaticClass(),
				Preset.PresetNode,
				FText::FromString(TEXT("Nodes")),
				FText::FromString(FString::Printf(TEXT("Create %s"), *Preset.PresetName)),
				FText::FromString(TEXT("Makes a new Dialogue node from preset")),
				0
			)
		);
		
		ContextMenuBuilder.AddAction(NewNodeAction);
	}
}

const FPinConnectionResponse UDialogueGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (A == nullptr || B == nullptr) {
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid connection parameters."));
	}
	if (A->Direction == B->Direction) {
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Cannot connect pins of same direction."));
	}

	if (A->Direction == EGPD_Output)
	{
		return	FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, FText::GetEmpty());
	}
	
	return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, FText::GetEmpty());
}

void UDialogueGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	UDialogueGraphNode_Start* StartNode = NewObject<UDialogueGraphNode_Start>(&Graph);
	StartNode->CreateNewGuid();
	StartNode->NodePosX = 0;
	StartNode->NodePosY = 0;
	StartNode->InitDialogueNode(nullptr);
	StartNode->OnGraphNodeCreated();
	StartNode->SyncPinsWithBranch();
    
	Graph.AddNode(StartNode, true, true);
	Graph.Modify();
}