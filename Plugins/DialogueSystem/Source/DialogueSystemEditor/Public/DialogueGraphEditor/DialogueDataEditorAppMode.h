// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

/**
 * 
 */
class FDialogueDataEditorApp;
class DIALOGUESYSTEMEDITOR_API FDialogueDataEditorAppMode : public FApplicationMode
{
public:
	FDialogueDataEditorAppMode(const TSharedPtr<FDialogueDataEditorApp>& InApp);

	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
	virtual void PreDeactivateMode() override;
	virtual void PostActivateMode() override;

protected:
	TWeakPtr<FDialogueDataEditorApp> EditorApp;
	FWorkflowAllowedTabSet Tabs;
};
