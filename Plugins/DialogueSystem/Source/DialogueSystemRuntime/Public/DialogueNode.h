// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogueFeatures/DNodeFeature_Branch.h"
#include "UObject/Object.h"
#include "DialogueNode.generated.h"

class UDNodeFeature;

UENUM()
enum class EDialogueNodeType : uint8
{
	None,
	StartNode,
	ContentNode,
	EndNode,
};

UCLASS(DefaultToInstanced, EditInlineNew, Blueprintable, BlueprintType)
class DIALOGUESYSTEMRUNTIME_API UDialogueNode : public UObject
{
	GENERATED_BODY()

public:
	UDialogueNode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/*~ UDialogueNode Interface ~*/
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	UDNodeFeature_Branch* FindBranch() const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta = (DeterminesOutputType = "FeatureClass"))
	UDNodeFeature* FindFeatureByClass(TSubclassOf<UDNodeFeature> FeatureClass) const;

	UFUNCTION(BlueprintCallable)
	virtual void ResetNode();
	
	template <typename ResultClass>
	ResultClass* FindFeatureByClass() const
	{
		return (ResultClass*)FindFeatureByClass(ResultClass::StaticClass());
	}
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DialogueNode")
	EDialogueNodeType NodeType = EDialogueNodeType::None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DialogueNode")
	FGameplayTag ParticipantTag;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DialogueNode")
	FName NodeID;
	
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "DialogueNode")
	TArray<UDNodeFeature*> NodeFeatures;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FVector2D Position;
#endif
	
private:
	friend class UDialogueManagerComponent;
};
