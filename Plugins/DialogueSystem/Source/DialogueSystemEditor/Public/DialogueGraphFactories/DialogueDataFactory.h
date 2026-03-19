// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DialogueDataFactory.generated.h"
/**
 * 
 */
UCLASS()
class DIALOGUESYSTEMEDITOR_API UDialogueDataFactory : public UFactory
{
	GENERATED_BODY()
	UDialogueDataFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/*~ UFactory Interface ~*/
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
	virtual bool CanCreateNew() const override;
};