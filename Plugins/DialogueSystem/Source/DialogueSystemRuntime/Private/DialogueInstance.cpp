// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueInstance.h"

#include "DialogueData.h"

void UDialogueInstance::SetDialogue(UDialogueData* InDialogueData)
{
	DialogueData = DuplicateObject(InDialogueData,GetOuter());

	IdToNodeMap.Empty();
	
	if (IsValid(DialogueData))
	{
		for (const TObjectPtr<UDialogueNode> Node : DialogueData->DialogueNodes)
		{
			IdToNodeMap.Add(Node->NodeID,Node);
		}
	}
}

UDialogueNode* UDialogueInstance::GetNodeById(const FName InNodeId)
{
	if (IdToNodeMap.Contains(InNodeId))
	{
		return IdToNodeMap[InNodeId];
	}
	return nullptr;
}
