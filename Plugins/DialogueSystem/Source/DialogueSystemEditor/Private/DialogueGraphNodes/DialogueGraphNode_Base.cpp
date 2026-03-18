// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemEditor/Public/DialogueGraphNodes/DialogueGraphNode_Base.h"
#include "DialogueFeatures/DNodeFeature_Branch.h"
#include "DialogueGraphEditor/DialogueDataEditorConfig.h"
#include "DialogueGraphEditor/EdDialogueGraph.h"
#include "DialogueGraphNodes/SDialogueGraphNode.h"
#include "DialogueSystemRuntime/Public/DialogueData.h"
#include "DialogueSystemRuntime/Public/DialogueNode.h"
#include "DialogueSystemRuntime/Public/DialogueSystemTypes.h"

struct FDialogueNodeLink;

void UDialogueGraphNode_Base::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->AddSection(TEXT("SectionName"), FText::FromString(TEXT("Dialogue Graph Node Actions")));
	UDialogueGraphNode_Base* Node = const_cast<UDialogueGraphNode_Base*>(this);
	
	if (CanUserDeleteNode())
	{
		Section.AddMenuEntry(
		TEXT("DeleteEntry"),
		FText::FromString(TEXT("Delete Node")),
		FText::FromString(TEXT("Deletes the node")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] ()
			{
				Node->GetGraph()->RemoveNode(Node);
			}))
		);
	}
}

FLinearColor UDialogueGraphNode_Base::GetNodeTitleColor() const
{
	if (IsValid(DialogueNode))
	{
		if (UDialogueData* DialogueData = GetDialogueData())
		{
			FDialogueParticipantInfo ParticipantInfo;
			if (DialogueData->DialogueParticipants.TryGetParticipantInfo(DialogueNode->ParticipantTag, ParticipantInfo))
			{
				return ParticipantInfo.ParticipantColor;
			}
		}
	}
	return FLinearColor(0.9f,0.5f,0.0f);
}

UEdGraphPin* UDialogueGraphNode_Base::CreateDialoguePin(EEdGraphPinDirection Direction, FName Name)
{
	FName Category = (Direction == EGPD_Input) ? TEXT("Inputs") : TEXT("Outputs");
	FName Subcategory = GetPinSubCategory();

	UEdGraphPin* Pin = CreatePin(
		Direction,
		Category,
		Name
	);
	Pin->PinType.PinSubCategory = Subcategory;

	return Pin;
}

void UDialogueGraphNode_Base::OnGraphNodeCreated()
{
	if (IsValid(DialogueNode))
	{
		if (UDNodeFeature_Branch* Branch = DialogueNode->FindBranch())
		{
			PinTextColor = Branch->GetBranchColor();
		}
	}
}

void UDialogueGraphNode_Base::OnDialogueNodePropertiesChanged()
{
	SyncPinsWithBranch();
}

void UDialogueGraphNode_Base::SyncPinsWithBranch()
{
	check(DialogueNode);

	int NumOutputPins = 0;

	for (UEdGraphPin* Pin : GetAllPins())
	{
		if (Pin->Direction==EGPD_Output)
		{
			NumOutputPins++;
		}
	}
	
	UDNodeFeature_Branch* Branch = DialogueNode->FindBranch();
	if (Branch == nullptr)
	{
		while (NumOutputPins > 0)
		{
			RemovePinAt(NumOutputPins - 1, EEdGraphPinDirection::EGPD_Output);
			NumOutputPins--;
		}
		return;
	}

	TArray<FDialogueNodeLink> Links = Branch->GetAllLinks();
	int NumLinks = Links.Num();

	while (NumOutputPins > NumLinks) {
		RemovePinAt(NumOutputPins - 1, EEdGraphPinDirection::EGPD_Output);
		NumOutputPins--;
	}
	
	while (NumLinks > NumOutputPins) {
		CreateDialoguePin(
			EEdGraphPinDirection::EGPD_Output,
			FName()
		);
		NumOutputPins++;
	}

	for (int PinIndex = 0; PinIndex < NumOutputPins; PinIndex++)
	{
		UEdGraphPin* Pin = GetPinWithDirectionAt(PinIndex, EEdGraphPinDirection::EGPD_Output);
		Pin->PinName = Links[PinIndex].LinkName;
	}
}

UDialogueData* UDialogueGraphNode_Base::GetDialogueData() const
{
	if ( UObject* Outer = GetGraph()->GetOuter()) // 그래프의 Outer는 DialogueData로 설정되어 있어야함.
	{
		return Cast<UDialogueData>(Outer);
	}
	return nullptr;
}

UDialogueNode* UDialogueGraphNode_Base::InitDialogueNode(const UDialogueNode* InPresetNode)
{
	UEdDialogueGraph* DialogueGraph = Cast<UEdDialogueGraph>(GetGraph());
	check(DialogueGraph);
	UDialogueDataEditorConfig* EditorConfig = DialogueGraph->GetDialogueDataEditorConfig();
	check(EditorConfig);
	UDialogueData* DialogueData = GetDialogueData();
	check(DialogueData);

	UDialogueNode* NewNode;
	if (InPresetNode != nullptr)
	{
		NewNode = DuplicateObject(InPresetNode,DialogueData);
	}
	else
	{
		NewNode = DuplicateObject(EditorConfig->GetEmptyNodePreset(),DialogueData);
	}

	check(NewNode != nullptr);
	if (NewNode == nullptr)
	{
		return nullptr;
	}

	int NumNode = GetGraph()->Nodes.Num();
	FName NodeID = FName(*FString::Printf(TEXT("Node_%d"), NumNode));
	NewNode->NodeID = NodeID;

	DialogueNode = NewNode;
	return NewNode;
}

TSharedPtr<SGraphNode> UDialogueGraphNode_Base::CreateVisualWidget()
{
	return TSharedPtr<SDialogueGraphNode>();
}
