// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemEditor/Public/DialogueGraphNodes/DialogueGraphNode_Content.h"

#include "DialogueData.h"
#include "DialogueSystemRuntime/Public/DialogueNode.h"

class UDNodeFeature_DefaultLinkBranch;
class UDNodeFeature_Text;

FText UDialogueGraphNode_Content::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!DialogueNode)
	{
		return FText();
	}

	FString TextString = FString::Printf(TEXT("(%s)"),*DialogueNode->NodeID.ToString());
	
	if (UDialogueData* DialogueData = GetDialogueData())
	{
		FDialogueParticipantInfo ParticipantInfo;
		if (DialogueData->DialogueParticipants.TryGetParticipantInfo(DialogueNode->ParticipantTag, ParticipantInfo))
		{
			TextString = ParticipantInfo.ParticipantID.ToString();
		}
	}
	
	return FText::FromString(TextString);
}

bool UDialogueGraphNode_Content::CanUserDeleteNode() const
{
	return true;
}

void UDialogueGraphNode_Content::OnGraphNodeCreated()
{
	Super::OnGraphNodeCreated();
	check(DialogueNode);

	DialogueNode->NodeType = EDialogueNodeType::ContentNode;
	CreateDialoguePin(EGPD_Input,FName());
}