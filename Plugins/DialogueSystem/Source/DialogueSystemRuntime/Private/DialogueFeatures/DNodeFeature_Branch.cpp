// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueFeatures/DNodeFeature_Branch.h"

void UDNodeFeature_Branch::SelectBranchIndex(const int32 InBranchIndex)
{
	SelectedBranchIndex = InBranchIndex;
	OnSelectBranchIndex(InBranchIndex);
}

void UDNodeFeature_Branch::OnSelectBranchIndex(const int32 InBranchIndex)
{
}

TArray<FDialogueNodeLink> UDNodeFeature_Branch::GetAllLinks() const
{
	return TArray<FDialogueNodeLink>();
}

void UDNodeFeature_Branch::UpdateLinks(const TArray<FDialogueNodeLink>& InLinks)
{
}
