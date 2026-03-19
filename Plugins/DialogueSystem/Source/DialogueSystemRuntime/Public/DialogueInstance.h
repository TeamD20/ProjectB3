// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DialogueInstance.generated.h"

class UDialogueNode;
class UDialogueData;
/**
 * 
 */
UCLASS(BlueprintType)
class DIALOGUESYSTEMRUNTIME_API UDialogueInstance : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SetDialogue(UDialogueData* InDialogueData);
	
	UFUNCTION(BlueprintCallable)
	UDialogueNode* GetNodeById(const FName InNodeId);
	
public:
	UPROPERTY()
	TObjectPtr<UDialogueData> DialogueData;
	
	UPROPERTY()
	TObjectPtr<UDialogueNode> CurrentDialogueNode = nullptr;
	
	UPROPERTY()
	TMap<FName, TObjectPtr<UDialogueNode>> IdToNodeMap;
};