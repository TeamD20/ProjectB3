// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogueNode.h"
#include "Engine/DataAsset.h"
#include "DialogueDataEditorConfig.generated.h"

/**
 * 
 */

USTRUCT()
struct FDialogueNodePreset
{
	GENERATED_BODY();

	UPROPERTY(EditDefaultsOnly)
	FString PresetName;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDialogueNode> PresetNode;
};

UCLASS()
class DIALOGUESYSTEMEDITOR_API UDialogueDataEditorConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UDialogueDataEditorConfig(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UDialogueNode* GetEmptyNodePreset() const {return EmptyNode;}

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "NodePreset")
	TSubclassOf<UDialogueNode> BaseNodeClass = UDialogueNode::StaticClass();

	UPROPERTY(EditDefaultsOnly, Category = "NodePreset")
	TArray<FDialogueNodePreset> Presets;

private:
	UPROPERTY()
	UDialogueNode* EmptyNode;
};