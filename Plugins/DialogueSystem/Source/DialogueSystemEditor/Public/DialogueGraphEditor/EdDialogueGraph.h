// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "EdGraph/EdGraph.h"
#include "EdDialogueGraph.generated.h"

class UDialogueDataEditorConfig;
/**
 * 
 */
UCLASS()
class DIALOGUESYSTEMEDITOR_API UEdDialogueGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	void SetDialogueDataEditorConfig(UDialogueDataEditorConfig* InConfig) {DialogueDataEditorConfig = InConfig;}
	UDialogueDataEditorConfig* GetDialogueDataEditorConfig() const {return DialogueDataEditorConfig;}

private:
	UPROPERTY()
	UDialogueDataEditorConfig* DialogueDataEditorConfig = nullptr;
};
