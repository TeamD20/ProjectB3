// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueManagerComponent.h"

#include "DialogueData.h"
#include "DialogueInstance.h"
#include "DialogueFeatures/DNodeFeature.h"
#include "DialogueFeatures/DNodeFeature_Branch.h"


UDialogueManagerComponent::UDialogueManagerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UDialogueManagerComponent::StartDialogue(UDialogueData* InDialogueData, const FDialogueSystemContext& InContext)
{
	PreStartDialogue(InDialogueData, InContext);
	SetCurrentDialogueData(InDialogueData, InContext);
	DialogueInstance = NewObject<UDialogueInstance>(this,UDialogueInstance::StaticClass());
	DialogueInstance->SetDialogue(CurrentDialogueData);
	SetCurrentNodeById(InDialogueData->StartNodeID);
}

void UDialogueManagerComponent::SetCurrentDialogueData(UDialogueData* InDialogueData, const FDialogueSystemContext& InContext)
{
	CurrentDialogueData = InDialogueData;
	CurrentDialogueContext = InContext;
}

void UDialogueManagerComponent::SetCurrentDialogueNode(UDialogueNode* InDialogueNode)
{
	UDialogueNode* PreviousNode = CurrentDialogueNode;
	CurrentDialogueNode = InDialogueNode;
	OnCurrentDialogueNodeSet(PreviousNode,CurrentDialogueNode);
	
	if (IsValid(CurrentDialogueNode))
	{
		for (auto NodeFeature : CurrentDialogueNode->NodeFeatures)
		{
			if (IsValid(NodeFeature))
			{
				NodeFeature->OnStartDialogueNode(CurrentDialogueNode, CurrentDialogueContext);	
			}
		}
		
		if (CurrentDialogueNode->NodeType == EDialogueNodeType::StartNode)
		{
			OnDialogueStart(CurrentDialogueNode);
			ProgressDialogue(0);
		}
		else if (CurrentDialogueNode->NodeType == EDialogueNodeType::EndNode)
		{
			OnDialogueEnd(CurrentDialogueNode);
			ProgressDialogue(0);
		}
		else
		{
			OnDialogueChanged(CurrentDialogueNode);
		}
	}
}

void UDialogueManagerComponent::ProgressDialogue(const int32 OptionIndex)
{
	if (!IsValid(CurrentDialogueNode))
	{
		OnDialogueEnd(nullptr);
		return;
	}

	if (!CanProgress(OptionIndex))
	{
		return;
	}

	// 현재 노드의 Feature들의 OnEndDialogueNode 호출
	for (auto NodeFeature : CurrentDialogueNode->NodeFeatures)
	{
		if (IsValid(NodeFeature))
		{
			NodeFeature->OnEndDialogueNode(CurrentDialogueNode, CurrentDialogueContext);
		}
	}
	
	if (CurrentDialogueNode->NodeType == EDialogueNodeType::EndNode)
	{
		SetCurrentDialogueData(nullptr, FDialogueSystemContext());
		SetCurrentDialogueNode(nullptr);
		return;
	}
	
	if (UDNodeFeature_Branch* Branch = CurrentDialogueNode->FindBranch())
	{
		Branch->SelectBranchIndex(OptionIndex);
		SetCurrentNodeById(Branch->GetNextNodeId(OptionIndex));
	}
}

bool UDialogueManagerComponent::CanProgress(const int32 OptionIndex)
{
	// TODO: NodeFeature 검사?
	return true;
}


void UDialogueManagerComponent::SetCurrentNodeById(const FName NodeID)
{
	if (UDialogueNode* Node = DialogueInstance->GetNodeById(NodeID))
	{
		SetCurrentDialogueNode(Node);
	}
}

void UDialogueManagerComponent::OnCurrentDialogueNodeSet(UDialogueNode* PreviousNode, UDialogueNode* NewNode)
{
	if (IsValid(PreviousNode))
	{
		DialogueHistory.PreviousNodes.Add(DuplicateObject(PreviousNode,this));
		PreviousNode->ResetNode();
	}
}

void UDialogueManagerComponent::OnDialogueStart(UDialogueNode* CurrentNode)
{
	OnDialogueStartDelegate.Broadcast(FDialogueChangeMessage(CurrentDialogueData,CurrentNode));
}

void UDialogueManagerComponent::OnDialogueChanged(UDialogueNode* CurrentNode)
{
	OnDialogueChangedDelegate.Broadcast(FDialogueChangeMessage(CurrentDialogueData,CurrentNode));
}

void UDialogueManagerComponent::OnDialogueEnd(UDialogueNode* CurrentNode)
{
	OnDialogueEndDelegate.Broadcast(FDialogueChangeMessage(CurrentDialogueData,CurrentNode));
}
