// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphFactories/DialogueDataEditorConfigTabFactory.h"
#include "DialogueGraphEditor/DialogueDataEditorApp.h"
#include "DialogueSystemEditor.h"
#include "DialogueGraphEditor/DialogueDataEditorConfig.h"


FDialogueDataEditorConfigTabFactory::FDialogueDataEditorConfigTabFactory(TSharedPtr<FDialogueDataEditorApp> InApp)
	:FWorkflowTabFactory(FName("DialogueDataEditorConfigTab"), InApp)
{
	EditorApp = InApp;
	TabLabel = FText::FromString(TEXT("Editor Config"));
	ViewMenuDescription = FText::FromString(TEXT("Displays the properties view for the current asset."));
	ViewMenuTooltip = FText::FromString(TEXT("Show the properties view."));
}
TSharedRef<SWidget> FDialogueDataEditorConfigTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FDialogueDataEditorApp> App = EditorApp.Pin();
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.NotifyHook = nullptr;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bShowModifiedPropertiesOption = false;
		DetailsViewArgs.bShowScrollBar = false;
	}

	TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	UDialogueDataEditorConfig* Config = FDialogueSystemEditorModule::GetDialogueDataEditorConfig();
	DetailsView->SetObject(Config);
	
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			DetailsView.ToSharedRef()
		];
}

FText FDialogueDataEditorConfigTabFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return FWorkflowTabFactory::GetTabToolTipText(Info);
}
