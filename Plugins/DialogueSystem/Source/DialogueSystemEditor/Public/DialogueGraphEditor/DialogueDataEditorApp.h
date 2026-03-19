// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "Toolkits/AssetEditorToolkit.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class UEdDialogueGraph;
class UDialogueGraphNode_Base;
class UDialogueData;
class DIALOGUESYSTEMEDITOR_API FDialogueDataEditorApp : public FWorkflowCentricApplication
{
public:
	/*~ FAssetEditorToolkit Interface ~*/
	virtual FName GetToolkitFName() const override { return FName("DialogueDataEditor"); }
	virtual FText GetBaseToolkitName() const override { return NSLOCTEXT("DialogueDataEditor", "AppLabel", "Dialogue Data Editor"); }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor::White; }
	virtual FString GetWorldCentricTabPrefix() const override { return FString("DialogueDataEditor"); }

	virtual void OnClose() override;
	
	/*~ FWorkflowCentricApplication Interface ~*/
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

	/*~ FDialogueDataEditor Interface ~*/
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDialogueData* InDialogueData);
	UDialogueData* GetWorkingAsset() const { return WorkingAsset; }
	UEdGraph* GetWorkingGraph() const;
	void SetWorkingGraphWidget(const TSharedPtr<SGraphEditor>& InWorkingGraphWidget) { WorkingGraphWidget = InWorkingGraphWidget; }
	void SetSelectedNodeDetailView(const TSharedPtr<class IDetailsView>& InDetailsView);
	// DialogueData 프로퍼티 뷰를 등록하고 변경 콜백을 바인딩
	void SetDataDetailView(const TSharedPtr<class IDetailsView>& InDetailsView);
	void OnGraphSelectionChanged(const FGraphPanelSelectionSet& Selection);
	
protected:
	void UpdateWorkingAssetFromGraph();
	void UpdateEditorGraphFromWorkingAsset();
	UDialogueGraphNode_Base* GetSelectedNode(const FGraphPanelSelectionSet& Selection);

private:
	void OnDataDetailViewPropertiesUpdated(const FPropertyChangedEvent& Event);
	void OnNodeDetailViewPropertiesUpdated(const FPropertyChangedEvent& Event);
	void OnWorkingAssetPreSave();
	
private:
	static const FName DialogueGraphEditorTabId;
	
	UPROPERTY()
	UDialogueData* WorkingAsset = nullptr;

	UPROPERTY()
	UEdDialogueGraph* WorkingGraph = nullptr;

	TSharedPtr<SGraphEditor> WorkingGraphWidget = nullptr;
	TSharedPtr<class IDetailsView> SelectedNodeDetailView = nullptr;
	
	FDelegateHandle GraphChangeListenerHandle;
};