// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphFactories/DialogueGraphNodeFactory.h"
#include "DialogueGraphNodes/DialogueGraphNode_Base.h"
#include "DialogueGraphNodes/SDialogueGraphNode.h"


TSharedPtr<SGraphNode> FDialogueGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	// UDialogueGraphNodeBase 타입에 대해서만 커스텀 위젯 생성
	if (UDialogueGraphNode_Base* DialogueNode = Cast<UDialogueGraphNode_Base>(Node))
	{
		return SNew(SDialogueGraphNode, DialogueNode);
	}
	return nullptr;
}


