// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DNodeFeature_Branch.h"
#include "DNodeFeature_DefaultLinkBranch.generated.h"

UCLASS(meta = (DisplayName = "Dialobue Branch: Default Link"))
class DIALOGUESYSTEMRUNTIME_API UDNodeFeature_DefaultLinkBranch : public UDNodeFeature_Branch
{
	GENERATED_BODY()
public:
	UDNodeFeature_DefaultLinkBranch(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/*~ UDialogueNodeFeature_Branch Interface ~*/
	virtual FName GetNextNodeId(const int32 OptionId) const override;
	virtual TArray<FDialogueNodeLink> GetAllLinks() const override;
	virtual void UpdateLinks(const TArray<FDialogueNodeLink>& InLinks) override;
	
public:
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Branch")
	FName NextNodeId;
};