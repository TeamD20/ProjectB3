// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphFactories/DialogueDataFactory.h"
#include "DialogueData.h"

UDialogueDataFactory::UDialogueDataFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = UDialogueData::StaticClass();
}

UObject* UDialogueDataFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	return NewObject<UDialogueData>(InParent, InName, Flags);
}

bool UDialogueDataFactory::CanCreateNew() const
{
	return true;
}
