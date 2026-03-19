// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogueSystemTypes.h"
#include "DialogueManagerComponent.generated.h"

struct FDialogueSystemContext;
class UDialogueNode;
class UDialogueInstance;
class UDialogueData;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueChangedSignature,FDialogueChangeMessage, DialogueChangeMessage);

// 대화 관리 매니저 컴포넌트, 주로 PlayerController에 부착
UCLASS(Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent))
class DIALOGUESYSTEMRUNTIME_API UDialogueManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDialogueManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/*~ UDialogueManagerComponent Interface ~*/
	UFUNCTION(BlueprintCallable, Category = "DialogueSystem")
	void StartDialogue(UDialogueData* InDialogueData, const FDialogueSystemContext& InContext);
	
	UFUNCTION(BlueprintCallable, Category = "DialogueSystem")
	void SetCurrentDialogueData(UDialogueData* InDialogueData, const FDialogueSystemContext& InContext);

	UFUNCTION(BlueprintCallable, Category = "DialogueSystem")
	void SetCurrentDialogueNode(UDialogueNode* InDialogueNode);
	
	UFUNCTION(BlueprintCallable, Category = "DialogueSystem")
	void ProgressDialogue(const int32 OptionIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "DialogueSystem")
	UDialogueData* GetCurrentDialogueData() const {return CurrentDialogueData;}

	UFUNCTION(BlueprintCallable, Category = "DialogueSystem")
	UDialogueNode* GetCurrentDialogueNode() const {return CurrentDialogueNode;}

	UFUNCTION(BlueprintCallable, Category = "DialogueSystem")
	UDialogueInstance* GetCurrentDialogueInstance() const {return DialogueInstance;}
	
protected:
	bool CanProgress(const int32 OptionIndex);
	void SetCurrentNodeById(const FName NodeID);
	virtual void PreStartDialogue(UDialogueData* InDialogueData, const FDialogueSystemContext& InContext){}
	virtual void OnCurrentDialogueNodeSet(UDialogueNode* PreviousNode, UDialogueNode* NewNode);
	virtual void OnDialogueStart(UDialogueNode* CurrentNode);
	virtual void OnDialogueChanged(UDialogueNode* CurrentNode);
	virtual void OnDialogueEnd(UDialogueNode* CurrentNode);
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnDialogueChangedSignature OnDialogueStartDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnDialogueChangedSignature OnDialogueChangedDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnDialogueChangedSignature OnDialogueEndDelegate;

protected:
	UPROPERTY()
    FDialogueHistory DialogueHistory;
	
private:
	UPROPERTY()
	TObjectPtr<UDialogueData> CurrentDialogueData = nullptr;

	UPROPERTY()
	FDialogueSystemContext CurrentDialogueContext;
	
	UPROPERTY()
	TObjectPtr<UDialogueInstance> DialogueInstance = nullptr;
	
	UPROPERTY()
	TObjectPtr<UDialogueNode> CurrentDialogueNode = nullptr;
};
