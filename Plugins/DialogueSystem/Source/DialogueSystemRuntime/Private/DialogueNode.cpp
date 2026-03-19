// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemRuntime/Public/DialogueNode.h"
#include "DialogueSystemRuntime/Public/DialogueFeatures/DNodeFeature.h"

UDialogueNode::UDialogueNode(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
}

UDNodeFeature_Branch* UDialogueNode::FindBranch() const
{
	return FindFeatureByClass<UDNodeFeature_Branch>();
}

UDNodeFeature* UDialogueNode::FindFeatureByClass(TSubclassOf<UDNodeFeature> FeatureClass) const
{
	if (FeatureClass != nullptr)
	{
		for (TObjectPtr<UDNodeFeature> Feature : NodeFeatures)
		{
			if (Feature && Feature->IsA(FeatureClass))
			{
				return Feature;
			}
		}
	}

	return nullptr;
}

void UDialogueNode::ResetNode()
{
}
