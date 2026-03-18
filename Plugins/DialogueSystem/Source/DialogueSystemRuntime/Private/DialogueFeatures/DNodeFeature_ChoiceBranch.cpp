// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueFeatures/DNodeFeature_ChoiceBranch.h"
#include "DialogueNode.h"

UDNodeFeature_ChoiceBranch::UDNodeFeature_ChoiceBranch(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

TArray<FText> UDNodeFeature_ChoiceBranch::GetChoiceTexts() const
{
	TArray<FText> Texts;

	for (const FDialogueChoice& Choice : Choices)
	{
		Texts.Add(Choice.ChoiceText);
	}

	return Texts;
}

FName UDNodeFeature_ChoiceBranch::GetNextNodeId(const int32 OptionId) const
{
	if (Choices.IsValidIndex(OptionId))
	{
		return Choices[OptionId].NextNodeID;
	}
	return FName();
}

TArray<FDialogueNodeLink> UDNodeFeature_ChoiceBranch::GetAllLinks() const
{
	TArray<FDialogueNodeLink> Links;

	for (const FDialogueChoice& Choice : Choices)
	{
		FDialogueNodeLink Link;
		Link.LinkName = FName(Choice.ChoiceText.ToString());
		Link.SourceNodeId = GetDialogueNode()->NodeID;
		Link.TargetNodeId = Choice.NextNodeID;
		Links.Add(Link);
	}

	return Links;
}

void UDNodeFeature_ChoiceBranch::UpdateLinks(const TArray<FDialogueNodeLink>& InLinks)
{
	if (Choices.Num() != InLinks.Num())
	{
		return;
	}

	for (int Index = 0; Index < Choices.Num(); Index++)
	{
		FDialogueChoice& Choice = Choices[Index];
		const FDialogueNodeLink& Link = InLinks[Index];
		
		Choice.NextNodeID = Link.TargetNodeId;
	}
}
