// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemEditor/Public/DialogueGraphNodes/DialogueGraphNode_Start.h"
#include "DialogueSystemRuntime/Public/DialogueData.h"
#include "DialogueSystemRuntime/Public/DialogueNode.h"
#include "DialogueSystemRuntime/Public/DialogueFeatures/DNodeFeature_DefaultLinkBranch.h"


class UDNodeFeature_DefaultLinkBranch;

void UDialogueGraphNode_Start::OnGraphNodeCreated()
{
	Super::OnGraphNodeCreated();

	check(DialogueNode);
	DialogueNode->NodeID = FName("StartNode");
	DialogueNode->NodeType = EDialogueNodeType::StartNode;

	if (!DialogueNode->FindBranch())
	{
		UDNodeFeature_DefaultLinkBranch* NextNodeFeature = NewObject<UDNodeFeature_DefaultLinkBranch>(DialogueNode);
		DialogueNode->NodeFeatures.Add(NextNodeFeature);
	}
	
	if (UDialogueData* DialogueData = GetDialogueData())
	{
		DialogueData->StartNodeID = DialogueNode->NodeID;
	}
}
