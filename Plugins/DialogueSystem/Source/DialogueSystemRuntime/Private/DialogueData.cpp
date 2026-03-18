// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemRuntime/Public/DialogueData.h"

#include "DialogueSystemRuntime.h"
#include "UObject/ObjectSaveContext.h"

UDialogueData::UDialogueData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UDialogueData::PreSave(FObjectPreSaveContext SaveContext)
{
	if (OnPreSaveListener)
	{
		OnPreSaveListener();
	}
	Super::PreSave(SaveContext);
}