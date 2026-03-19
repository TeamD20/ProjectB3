// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemEditor/Public/DialogueGraphNodes/DialogueGraphNode_End.h"
#include "DialogueNode.h"

void UDialogueGraphNode_End::OnGraphNodeCreated()
{
	Super::OnGraphNodeCreated();
	check(DialogueNode);

	DialogueNode->NodeType = EDialogueNodeType::EndNode;
	CreateDialoguePin(EGPD_Input,FName());
}
