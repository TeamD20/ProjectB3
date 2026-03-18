// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemRuntime/Public/DialogueFeatures/DNodeFeature_DefaultLinkBranch.h"
#include "DialogueSystemRuntime/Public/DialogueNode.h"

UDNodeFeature_DefaultLinkBranch::UDNodeFeature_DefaultLinkBranch(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

FName UDNodeFeature_DefaultLinkBranch::GetNextNodeId(const int32 OptionId) const
{
	return NextNodeId;
}

TArray<FDialogueNodeLink> UDNodeFeature_DefaultLinkBranch::GetAllLinks() const
{
	const UDialogueNode* DialogueNode = GetDialogueNode();
	check(DialogueNode!= nullptr);
	
	TArray<FDialogueNodeLink> Links;
	{
		FDialogueNodeLink Link;
		Link.LinkName = FName("Next");
		Link.SourceNodeId = DialogueNode->NodeID;
		Link.TargetNodeId = NextNodeId;
		Links.Add(Link);
	}
	
	return Links;
}

void UDNodeFeature_DefaultLinkBranch::UpdateLinks(const TArray<FDialogueNodeLink>& InLinks)
{
	if (InLinks.IsEmpty())
	{
		NextNodeId = FName();
	}
	else
	{
		NextNodeId = InLinks[0].TargetNodeId;
	}
}
