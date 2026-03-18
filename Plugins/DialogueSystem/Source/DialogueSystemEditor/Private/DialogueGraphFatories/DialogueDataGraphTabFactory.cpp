// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphFactories/DialogueDataGraphTabFactory.h"
#include "DialogueGraphEditor/DialogueDataEditorApp.h"

FDialogueDataGraphTabFactory::FDialogueDataGraphTabFactory(const TSharedPtr<FDialogueDataEditorApp>& InApp):
	FWorkflowTabFactory(FName("DialogueDataGraphTab"),InApp)
{
	EditorApp = InApp;
	TabLabel = FText::FromString(TEXT("Dialogue Graph"));
	ViewMenuDescription = FText::FromString(TEXT("Displays a primary view for whatever you want to do."));
	ViewMenuTooltip = FText::FromString(TEXT("Show the primary view."));
}

TSharedRef<SWidget> FDialogueDataGraphTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FDialogueDataEditorApp> App = EditorApp.Pin();

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnSelectionChanged.BindRaw(App.Get(), &FDialogueDataEditorApp::OnGraphSelectionChanged);

	TSharedPtr<SGraphEditor> GraphEditor = 
		SNew(SGraphEditor)
			.IsEditable(true)
			.GraphEvents(GraphEvents)
			.GraphToEdit(Cast<UEdGraph>( App->GetWorkingGraph()));
	App->SetWorkingGraphWidget(GraphEditor);

	return SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.HAlign(HAlign_Fill)
				[
					GraphEditor.ToSharedRef()
				];
}

FText FDialogueDataGraphTabFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return NSLOCTEXT("DialogueDataEditor", "DialogueGraphEditorTabTooltip", "Edit the dialogue graph");
}
