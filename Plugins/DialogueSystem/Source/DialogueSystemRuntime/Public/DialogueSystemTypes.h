// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogueSystemTypes.generated.h"


class UDialogueNode;
class UDialogueData;

USTRUCT(BlueprintType)
struct FDialogueSystemContext
{
	GENERATED_BODY()
	
	// TODO: 대화 Feature에 필요한 Context 정보 기입
};

USTRUCT(BlueprintType)
struct FDialogueParticipantInfo
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DialogueGraph")
	FName ParticipantID;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DialogueGraph")
	FColor ParticipantColor;
};

USTRUCT(BlueprintType)
struct FDialogueParticipants
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DialogueGraph")
	TMap<FGameplayTag, FDialogueParticipantInfo> ParticipantInfoMap;
	
public:
	bool TryGetParticipantInfo(const FGameplayTag& InParticipantTag, FDialogueParticipantInfo& OutInfo)
	{
		if (ParticipantInfoMap.Contains(InParticipantTag))
		{
			OutInfo = ParticipantInfoMap[InParticipantTag];
			return true;
		}
		
		return false;
	}
};


USTRUCT(BlueprintType)
struct FDialogueChangeMessage
{
	GENERATED_BODY()

public:
	FDialogueChangeMessage()
	{
	}
	FDialogueChangeMessage(UDialogueData* InDialogueData, UDialogueNode* InDialogueNode)
		: DialogueData(InDialogueData), CurrentNode(InDialogueNode)
	{
	}

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UDialogueData> DialogueData = nullptr;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UDialogueNode> CurrentNode = nullptr;
};

USTRUCT()
struct FDialogueHistory
{
	GENERATED_BODY()
public:
	FDialogueHistory(){}
	
public:
	UPROPERTY()
	TArray<UDialogueNode*> PreviousNodes;
};