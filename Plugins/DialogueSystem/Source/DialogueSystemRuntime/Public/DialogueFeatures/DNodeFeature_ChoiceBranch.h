// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DNodeFeature_Branch.h"
#include "DNodeFeature_ChoiceBranch.generated.h"

USTRUCT(BlueprintType)
struct FDialogueChoice
{
	GENERATED_BODY()
	FDialogueChoice(){}

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText ChoiceText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName NextNodeID;
};

UCLASS(Abstract, BlueprintType, meta = (DisplayName = "Dialogue Branch: User Choice"))
class DIALOGUESYSTEMRUNTIME_API UDNodeFeature_ChoiceBranch : public UDNodeFeature_Branch
{
	GENERATED_BODY()

public:
	UDNodeFeature_ChoiceBranch(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/*~ UDialogueNodeFeature_ChoiceBranch Interface ~*/
	UFUNCTION(BlueprintCallable)
	virtual TArray<FText> GetChoiceTexts() const;

	/*~ UDialogueNodeFeature_Branch Interface ~*/
	virtual FName GetNextNodeId(const int32 OptionId) const override;
	virtual TArray<FDialogueNodeLink> GetAllLinks() const override;
	virtual void UpdateLinks(const TArray<FDialogueNodeLink>& InLinks) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FDialogueChoice> Choices;
};