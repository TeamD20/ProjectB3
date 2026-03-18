// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "DNodeFeature.h"
#include "DNodeFeature_Branch.generated.h"

USTRUCT(BlueprintType)
struct FDialogueNodeLink
{
	GENERATED_BODY()
	FDialogueNodeLink(){}
public:
	UPROPERTY()
	FName LinkName;
	
	UPROPERTY()
	FName SourceNodeId;
	
	UPROPERTY()
	FName TargetNodeId;
};

UCLASS(Abstract)
class DIALOGUESYSTEMRUNTIME_API UDNodeFeature_Branch : public UDNodeFeature
{
	GENERATED_BODY()

public:
	void SelectBranchIndex(const int32 InBranchIndex = 0);
	int32 GetSelectedBranchIndex() const {return SelectedBranchIndex;}
	
	virtual FName GetNextNodeId(const int32 InBranchIndex) const PURE_VIRTUAL(UDNodeFeature_Branch::GetNextNodeId, return FName(););

	// 그래프 에디터에 사용되는 함수
	virtual TArray<FDialogueNodeLink> GetAllLinks() const;
	virtual void UpdateLinks(const TArray<FDialogueNodeLink>& InLinks);
	virtual FLinearColor GetBranchColor() const { return FLinearColor::White; }
	
protected:
	virtual void OnSelectBranchIndex(const int32 InBranchIndex);
	
private:
	int32 SelectedBranchIndex = -1;
};
