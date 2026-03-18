// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphEditor/DialogueDataEditorConfig.h"

UDialogueDataEditorConfig::UDialogueDataEditorConfig(const FObjectInitializer& ObjectInitializer)
{
	if (BaseNodeClass != nullptr)
	{
		EmptyNode = Cast<UDialogueNode>(ObjectInitializer.CreateDefaultSubobject(this,TEXT("Empty Node"),UDialogueNode::StaticClass(),BaseNodeClass));
	}
	if (Presets.IsEmpty())
	{
		FDialogueNodePreset DefaultPreset;
		DefaultPreset.PresetName = "Default Node";
		DefaultPreset.PresetNode = Cast<UDialogueNode>(ObjectInitializer.CreateDefaultSubobject(this,TEXT("Default Node"),UDialogueNode::StaticClass(),BaseNodeClass));
		Presets.Add(DefaultPreset);
	}
}

#if WITH_EDITOR
void UDialogueDataEditorConfig::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	FName PropertyName = (PropertyChangedEvent.Property != nullptr)? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UDialogueDataEditorConfig, BaseNodeClass))
	{
		// BaseNodeClass가 변경되었으므로 EmptyNode를 새 인스턴스로 생성
		if (BaseNodeClass)
		{
			EmptyNode = NewObject<UDialogueNode>(this, BaseNodeClass);
		}
	}
}
#endif
