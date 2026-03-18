// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "DialogueNode.h"
#include "Engine/DataAsset.h"
#include "DialogueData.generated.h"

class UDNodeFeature;
class UDialogueNode;
class UDialogueGraph;

UCLASS(Blueprintable, BlueprintType)
class DIALOGUESYSTEMRUNTIME_API UDialogueData : public UDataAsset
{
	GENERATED_BODY()

public:
	UDialogueData(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/*~ UObject Interface ~*/
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	
	/*~ UDialogueData Interface ~*/
	void BindPreSaveListener(const TFunction<void()>& PreSaveListener) { OnPreSaveListener = PreSaveListener; }

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DialogueGraph")
	FDialogueParticipants DialogueParticipants;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DialogueGraph")
	FName StartNodeID;

	UPROPERTY(BlueprintReadOnly, Instanced, Category = "DialogueGraph")
	TArray<TObjectPtr<UDialogueNode>> DialogueNodes;

private:
	TFunction<void()> OnPreSaveListener;
};