// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DialogueSystemTypes.h"
#include "DNodeFeature.generated.h"

class UDialogueNode;

UCLASS(DefaultToInstanced, EditInlineNew, Abstract, Blueprintable)
class DIALOGUESYSTEMRUNTIME_API UDNodeFeature : public UObject
{
	GENERATED_BODY()
	
public:
	UDNodeFeature(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "DialogueSystem")
	void OnStartDialogueNode(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "DialogueSystem")
	void OnEndDialogueNode(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext);
	
	UFUNCTION(BlueprintCallable,Category="DialogueSystem")
	UDialogueNode* GetDialogueNode() const;
};