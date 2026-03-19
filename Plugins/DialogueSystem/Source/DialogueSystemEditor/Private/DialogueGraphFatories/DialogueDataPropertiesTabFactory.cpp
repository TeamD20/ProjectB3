// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphFactories/DialogueDataPropertiesTabFactory.h"
#include "DialogueData.h"
#include "DialogueGraphEditor/DialogueDataEditorApp.h"


FDialogueDataPropertiesTabFactory::FDialogueDataPropertiesTabFactory(TSharedPtr<FDialogueDataEditorApp> InApp) :
	FWorkflowTabFactory(FName("DialogueDataPropertiesTab"), InApp)
{
	EditorApp = InApp;
	TabLabel = FText::FromString(TEXT("Dialogue Data Properties"));
	ViewMenuDescription = FText::FromString(TEXT("Displays the properties view for the current asset."));
	ViewMenuTooltip = FText::FromString(TEXT("Show the properties view."));
}

TSharedRef<SWidget> FDialogueDataPropertiesTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FDialogueDataEditorApp> App = EditorApp.Pin();
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	
	FDetailsViewArgs GraphViewArgs;
	{
		GraphViewArgs.bAllowSearch = false;
		GraphViewArgs.bHideSelectionTip = true;
		GraphViewArgs.bLockable = false;
		GraphViewArgs.bSearchInitialKeyFocus = true;
		GraphViewArgs.bUpdatesFromSelection = false;
		GraphViewArgs.NotifyHook = nullptr;
		GraphViewArgs.bShowOptions = true;
		GraphViewArgs.bShowModifiedPropertiesOption = false;
		GraphViewArgs.bShowScrollBar = false;
	}

	FDetailsViewArgs NodeViewArgs;
	{
		NodeViewArgs.bAllowSearch = true;
		NodeViewArgs.bHideSelectionTip = true;
		NodeViewArgs.bLockable = false;
		NodeViewArgs.bSearchInitialKeyFocus = true;
		NodeViewArgs.bUpdatesFromSelection = false;
		NodeViewArgs.NotifyHook = nullptr;
		NodeViewArgs.bShowOptions = true;
		NodeViewArgs.bShowModifiedPropertiesOption = false;
		NodeViewArgs.bShowScrollBar = false;
	}
	
	TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(GraphViewArgs);
	UDialogueData* Asset = App->GetWorkingAsset();
	DetailsView->SetObject(Asset);
	App->SetDataDetailView(DetailsView);

	TSharedPtr<IDetailsView> SelectedNodeDetailsView = PropertyEditorModule.CreateDetailView(NodeViewArgs);
	SelectedNodeDetailsView->SetObject(nullptr);
	App->SetSelectedNodeDetailView(SelectedNodeDetailsView);
	
	return SNew(SVerticalBox)
		// 상단: DialogueData Detail
		+ SVerticalBox::Slot()
		.FillHeight(0.6f)
		.HAlign(HAlign_Fill)
		[
			DetailsView.ToSharedRef()
		]
		// 하단: DialogueNode Detail
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			SelectedNodeDetailsView.ToSharedRef()
		];
}

FText FDialogueDataPropertiesTabFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return FText::FromString(TEXT("A properties view for the current asset."));
}