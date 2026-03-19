// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphEditor/DialogueDataEditorAppMode.h"
#include "DialogueGraphFactories/DialogueDataGraphTabFactory.h"
#include "DialogueGraphFactories/DialogueDataPropertiesTabFactory.h"
#include "DialogueGraphEditor/DialogueDataEditorApp.h"
#include "DialogueGraphFactories/DialogueDataEditorConfigTabFactory.h"


FDialogueDataEditorAppMode::FDialogueDataEditorAppMode(const TSharedPtr<FDialogueDataEditorApp>& InApp) : FApplicationMode(TEXT("DialogueDataEditorAppMode"))
{
	EditorApp = InApp;
	Tabs.RegisterFactory(MakeShareable(new FDialogueDataGraphTabFactory(InApp)));
	Tabs.RegisterFactory(MakeShareable(new FDialogueDataPropertiesTabFactory(InApp)));
	Tabs.RegisterFactory(MakeShareable(new FDialogueDataEditorConfigTabFactory(InApp)));

	TabLayout = FTabManager::NewLayout("DialogueDataEditorAppMode_Layout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewSplitter()
						->SetOrientation(Orient_Horizontal)
						->Split
						(
							FTabManager::NewStack()
								->SetSizeCoefficient(0.75)
								->AddTab(FName(TEXT("DialogueDataGraphTab")), ETabState::OpenedTab)
						)
						->Split
						(
							FTabManager::NewStack()
								->SetSizeCoefficient(0.25)
								->AddTab(FName(TEXT("DialogueDataPropertiesTab")), ETabState::OpenedTab)
								->AddTab(FName(TEXT("DialogueDataEditorConfigTab")),ETabState::ClosedTab)
								->SetForegroundTab(FName(TEXT("DialogueDataPropertiesTab")))
						)
				)
		);
}

void FDialogueDataEditorAppMode::RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager)
{
	TSharedPtr<FDialogueDataEditorApp> App = EditorApp.Pin();
	App->PushTabFactories(Tabs);
	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FDialogueDataEditorAppMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();
}

void FDialogueDataEditorAppMode::PostActivateMode()
{
	FApplicationMode::PostActivateMode();
}
