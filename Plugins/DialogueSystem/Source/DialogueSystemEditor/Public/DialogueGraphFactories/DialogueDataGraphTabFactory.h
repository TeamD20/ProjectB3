// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

/**
 * 
 */
class FDialogueDataEditorApp;

class DIALOGUESYSTEMEDITOR_API FDialogueDataGraphTabFactory : public FWorkflowTabFactory
{
public:
	FDialogueDataGraphTabFactory(const TSharedPtr<FDialogueDataEditorApp>& InApp);

	/*~ FWorkflowTabFactory Interface ~*/
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FDialogueDataEditorApp> EditorApp;
};