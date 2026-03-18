// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphEditor/DialogueDataEditorApp.h"
#include "DialogueSystemEditor.h"
#include "DialogueFeatures/DNodeFeature_Branch.h"
#include "DialogueGraphEditor/DialogueDataEditorConfig.h"
#include "DialogueGraphEditor/EdDialogueGraph.h"
#include "DialogueGraphNodes/DialogueGraphNode_End.h"
#include "DialogueGraphEditor/DialogueDataEditorAppMode.h"
#include "DialogueGraphEditor/DialogueGraphSchema.h"
#include "DialogueGraphNodes/DialogueGraphNode_Content.h"
#include "DialogueGraphNodes/DialogueGraphNode_Base.h"
#include "DialogueGraphNodes/DialogueGraphNode_Start.h"
#include "DialogueSystemRuntime/Public/DialogueData.h"
#include "DialogueSystemRuntime/Public/DialogueNode.h"
#include "Kismet2/BlueprintEditorUtils.h"


struct FDialogueNodeLink;
class FDialogueDataGraphTabFactory;
const FName FDialogueDataEditorApp::DialogueGraphEditorTabId(TEXT("DialogueGraphEditor"));

void FDialogueDataEditorApp::OnClose()
{
	UpdateWorkingAssetFromGraph();
	WorkingAsset->BindPreSaveListener(nullptr);
	FWorkflowCentricApplication::OnClose();
}

void FDialogueDataEditorApp::OnDataDetailViewPropertiesUpdated(const FPropertyChangedEvent& Event)
{
	// DialogueData(참여자 정보 등) 변경 시 모든 그래프 노드 갱신
	if (WorkingGraphWidget != nullptr)
	{
		WorkingGraphWidget->NotifyGraphChanged();
	}
}

void FDialogueDataEditorApp::OnNodeDetailViewPropertiesUpdated(const FPropertyChangedEvent& Event)
{
	if (WorkingGraphWidget != nullptr)
	{
		UDialogueGraphNode_Base* DialogueGraphNode = GetSelectedNode(WorkingGraphWidget->GetSelectedNodes());
		if (DialogueGraphNode != nullptr) {
			DialogueGraphNode->OnDialogueNodePropertiesChanged();
		}
		WorkingGraphWidget->NotifyGraphChanged();
	}
}

void FDialogueDataEditorApp::OnWorkingAssetPreSave()
{
	UpdateWorkingAssetFromGraph();
}

void FDialogueDataEditorApp::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::RegisterTabSpawners(InTabManager);
}

void FDialogueDataEditorApp::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	InTabManager->UnregisterTabSpawner(DialogueGraphEditorTabId);
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);
}

void FDialogueDataEditorApp::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UDialogueData* InDialogueData)
{
	TArray<UObject*> ObjectsToEdit;
	ObjectsToEdit.Add(InDialogueData);
    
	WorkingAsset = InDialogueData;
	WorkingAsset->BindPreSaveListener([this](){OnWorkingAssetPreSave();});
	
	// Create an editor graph
	WorkingGraph = Cast<UEdDialogueGraph>( FBlueprintEditorUtils::CreateNewGraph(
		WorkingAsset, 
		NAME_None, 
		UEdDialogueGraph::StaticClass(), 
		UDialogueGraphSchema::StaticClass()
	));

	if (UDialogueDataEditorConfig* Config = FDialogueSystemEditorModule::GetDialogueDataEditorConfig())
	{
		WorkingGraph->SetDialogueDataEditorConfig(Config);
	}

	InitAssetEditor( 
		Mode, 
		InitToolkitHost, 
		TEXT("DialogueDataEditor"), 
		FTabManager::FLayout::NullLayout, 
		true, // createDefaultStandaloneMenu 
		true,  // createDefaultToolbar
		ObjectsToEdit);

	// Add our modes (just one for this example)
	AddApplicationMode(TEXT("DialogueDataEditorAppMode"), MakeShareable(new FDialogueDataEditorAppMode(SharedThis(this))));

	// Set the mode
	SetCurrentMode(TEXT("DialogueDataEditorAppMode"));

	// Load the UI from the asset
	UpdateEditorGraphFromWorkingAsset();
}

UEdGraph* FDialogueDataEditorApp::GetWorkingGraph() const
{
	return Cast<UEdGraph>( WorkingGraph); 
}

void FDialogueDataEditorApp::SetSelectedNodeDetailView(const TSharedPtr<class IDetailsView>& InDetailsView)
{
	SelectedNodeDetailView = InDetailsView;
	SelectedNodeDetailView->OnFinishedChangingProperties().AddRaw(this, &FDialogueDataEditorApp::OnNodeDetailViewPropertiesUpdated);
}

void FDialogueDataEditorApp::SetDataDetailView(const TSharedPtr<IDetailsView>& InDetailsView)
{
	InDetailsView->OnFinishedChangingProperties().AddRaw(this, &FDialogueDataEditorApp::OnDataDetailViewPropertiesUpdated);
}

void FDialogueDataEditorApp::OnGraphSelectionChanged(const FGraphPanelSelectionSet& Selection)
{
	UDialogueGraphNode_Base* SelectedNode = GetSelectedNode(Selection);
	if (SelectedNode != nullptr)
	{
		SelectedNodeDetailView->SetObject(SelectedNode->DialogueNode);
		SelectedNodeDetailView->ShowAllAdvancedProperties();
	}
	else
	{
		SelectedNodeDetailView->SetObject(nullptr);
	}
}

void FDialogueDataEditorApp::UpdateWorkingAssetFromGraph()
{
	if (WorkingGraph == nullptr || WorkingAsset == nullptr)
	{
		return;
	}

	UDialogueDataEditorConfig* Config = WorkingGraph->GetDialogueDataEditorConfig();
	WorkingAsset->DialogueNodes.Reset();
	
	for (TObjectPtr<class UEdGraphNode> Node : WorkingGraph->Nodes)
	{
		UDialogueGraphNode_Base* GraphNode = Cast<UDialogueGraphNode_Base>(Node);
		UDialogueNode* DialogueNode = GraphNode->DialogueNode;
		if (GraphNode == nullptr || DialogueNode == nullptr)
		{
			continue;
		}

		// BaseNode클래스의 변경이 있는 경우 노드가 해당 클래스 타입이 아니라면
		if (Config != nullptr && Config->BaseNodeClass != nullptr && !DialogueNode->IsA(Config->BaseNodeClass))
		{
			// BaseNode 클래스로 교체
			DialogueNode = NewObject<UDialogueNode>(WorkingAsset,Config->BaseNodeClass);
			DialogueNode->NodeID = GraphNode->DialogueNode->NodeID;
			DialogueNode->NodeFeatures = GraphNode->DialogueNode->NodeFeatures;
			GraphNode->DialogueNode = DialogueNode;
		}
		
		// Branch가 있는 노드의 경우
		if ( UDNodeFeature_Branch* Branch = DialogueNode->FindBranch())
		{
			// 그래프의 연결을 Link로 저장
			TArray<FDialogueNodeLink> Links;
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				// Output -> Input으로 들어가는 연결만 추적
				if (Pin->Direction != EGPD_Output)
				{
					continue;
				} 
				
				FDialogueNodeLink Link;
				Link.LinkName = Pin->PinName;
				Link.SourceNodeId = DialogueNode->NodeID;
				if (!Pin->LinkedTo.IsEmpty())
				{
					UDialogueGraphNode_Base* Linked = Cast<UDialogueGraphNode_Base>(Pin->LinkedTo[0]->GetOwningNode());
					if (Linked  != nullptr && Linked->DialogueNode != nullptr)
					{
						Link.TargetNodeId = Linked->DialogueNode->NodeID;
					}
				}
				else
				{
					// 다른 핀과 연결이 되어 있지 않으면 경고
					UE_LOG(LogTemp, Warning, TEXT("There is no link at pin %s of node(%s)"),*Pin->PinName.ToString(),*DialogueNode->NodeID.ToString());
				}
				
				Links.Add(Link);
			}
			Branch->UpdateLinks(Links);
		}

		DialogueNode->Position = FVector2D(GraphNode->NodePosX, GraphNode->NodePosY);
		WorkingAsset->DialogueNodes.Add(DuplicateObject<UDialogueNode>(DialogueNode, WorkingAsset));
	}
}

void FDialogueDataEditorApp::UpdateEditorGraphFromWorkingAsset()
{
	if (WorkingAsset->DialogueNodes.IsEmpty())
	{
		WorkingGraph->GetSchema()->CreateDefaultNodesForGraph(*WorkingGraph);
		return;
	}

	TMap<FName,UDialogueGraphNode_Base*> IdToGraphNodeMap;
	TArray<FDialogueNodeLink> Links;
	for (UDialogueNode* DialogueNode : WorkingAsset->DialogueNodes)
	{
		if (DialogueNode == nullptr)
		{
			continue;
		}
		
		UDialogueGraphNode_Base* NewGraphNode = nullptr;

		switch (DialogueNode->NodeType)
		{
		case EDialogueNodeType::StartNode:
			NewGraphNode = NewObject<UDialogueGraphNode_Start>(WorkingGraph);
			break;
		case EDialogueNodeType::EndNode:
			NewGraphNode = NewObject<UDialogueGraphNode_End>(WorkingGraph);
			break;
		case EDialogueNodeType::ContentNode:
			NewGraphNode = NewObject<UDialogueGraphNode_Content>(WorkingGraph);
			break;
		default:
			NewGraphNode = NewObject<UDialogueGraphNode_Base>(WorkingGraph);
		}

		NewGraphNode->DialogueNode = DialogueNode;
		NewGraphNode->OnGraphNodeCreated();
		NewGraphNode->CreateNewGuid();
		NewGraphNode->NodePosX = DialogueNode->Position.X;
		NewGraphNode->NodePosY = DialogueNode->Position.Y;
		NewGraphNode->DialogueNode = DialogueNode;

		WorkingGraph->AddNode(NewGraphNode);
		
		if (IdToGraphNodeMap.Contains(DialogueNode->NodeID))
		{
			UE_LOG(LogTemp,Warning,TEXT("DialogueData %s has duplicated node id %s"),*WorkingAsset->GetName(),*DialogueNode->NodeID.ToString())
		}
		else
		{
			IdToGraphNodeMap.Add(DialogueNode->NodeID, NewGraphNode);
			if (UDNodeFeature_Branch* Branch = DialogueNode->FindBranch())
			{
				Links.Append(Branch->GetAllLinks());
			}
		}
	}

	for (FDialogueNodeLink& Link : Links)
	{
		UDialogueGraphNode_Base* SourceNode = nullptr;
		UDialogueGraphNode_Base* TargetNode = nullptr;

		if (IdToGraphNodeMap.Contains(Link.SourceNodeId))
		{
			SourceNode = IdToGraphNodeMap[Link.SourceNodeId];
		}
		
		if (IdToGraphNodeMap.Contains(Link.TargetNodeId))
		{
			TargetNode = IdToGraphNodeMap[Link.TargetNodeId];
		}

		UEdGraphPin* OutputPin = nullptr;
		UEdGraphPin* InputPin = nullptr;
		
		if (SourceNode != nullptr)
		{
			OutputPin = SourceNode->CreateDialoguePin(EGPD_Output,Link.LinkName);
		}
		
		if (TargetNode != nullptr)
		{
			if (UEdGraphPin* Pin = TargetNode->GetPinWithDirectionAt(0, EGPD_Input))
			{
				InputPin = Pin;
			}
			else
			{
				InputPin = TargetNode->CreateDialoguePin(EGPD_Input,FName());
			}
		}

		if (OutputPin != nullptr && InputPin != nullptr)
		{
			OutputPin->LinkedTo.Add(InputPin);
			InputPin->LinkedTo.Add(OutputPin);
		}
	}
}

UDialogueGraphNode_Base* FDialogueDataEditorApp::GetSelectedNode(const FGraphPanelSelectionSet& Selection)
{
	for (UObject* Object : Selection)
	{
		if (UDialogueGraphNode_Base* GraphNode = Cast<UDialogueGraphNode_Base>(Object))
		{
			return GraphNode;
		}
	}

	return nullptr;
}
