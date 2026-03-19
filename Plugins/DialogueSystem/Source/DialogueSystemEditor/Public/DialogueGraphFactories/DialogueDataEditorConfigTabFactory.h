// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FDialogueDataEditorApp;
/**
 * 
 */
class DIALOGUESYSTEMEDITOR_API FDialogueDataEditorConfigTabFactory: public FWorkflowTabFactory
{
public:
	FDialogueDataEditorConfigTabFactory(TSharedPtr<FDialogueDataEditorApp> InApp);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FDialogueDataEditorApp> EditorApp;
};
