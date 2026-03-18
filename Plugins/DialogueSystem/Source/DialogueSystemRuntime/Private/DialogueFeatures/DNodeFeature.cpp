// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemRuntime/Public/DialogueFeatures/DNodeFeature.h"
#include "DialogueSystemRuntime/Public/DialogueNode.h"

UDNodeFeature::UDNodeFeature(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UDNodeFeature::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode,
	const FDialogueSystemContext& InDialogueContext)
{
}


void UDNodeFeature::OnEndDialogueNode_Implementation(const UDialogueNode* InDialogueNode,
	const FDialogueSystemContext& InDialogueContext)
{
}

UDialogueNode* UDNodeFeature::GetDialogueNode() const
{
	if (UObject* Outer = GetOuter())
	{
		return Cast<UDialogueNode>(Outer);
	}
	return nullptr;
}
