// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogueSystemTypes.generated.h"

class UDialogueData;
class UDialogueNode;

USTRUCT(BlueprintType)
struct DIALOGUESYSTEMRUNTIME_API FDialogueSystemContext
{
    GENERATED_BODY()

    // 프로젝트 레벨 컨텍스트 오브젝트
    // Manager가 StartDialogue 시 설정하고, 프로젝트 Feature가 캐스팅하여 사용한다.
    TWeakObjectPtr<UObject> ContextObject;

    // 대화를 시작한 플레이어 Pawn
    TWeakObjectPtr<AActor> InstigatorActor;

    // 대화 대상 NPC Actor
    TWeakObjectPtr<AActor> TargetActor;

    // 대화를 시작한 PlayerController
    TWeakObjectPtr<APlayerController> InstigatorController;

    // ParticipantTag : Actor 매핑.
    TMap<FGameplayTag, TWeakObjectPtr<AActor>> ParticipantActors;

    // ContextObject를 지정 타입으로 캐스팅하여 반환
    template<typename T>
    T* GetContextObject() const { return Cast<T>(ContextObject.Get()); }

    // ParticipantTag에 해당하는 Actor를 등록
    void RegisterParticipantActor(const FGameplayTag& InTag, AActor* InActor)
    {
        if (InTag.IsValid() && IsValid(InActor))
        {
            ParticipantActors.Add(InTag, InActor);
        }
    }

    // ParticipantTag에 해당하는 Actor를 반환. 없으면 nullptr
    AActor* FindParticipantActor(const FGameplayTag& InTag) const
    {
        if (const TWeakObjectPtr<AActor>* Found = ParticipantActors.Find(InTag))
        {
            return Found->Get();
        }
        return nullptr;
    }
};

USTRUCT(BlueprintType)
struct DIALOGUESYSTEMRUNTIME_API FDialogueParticipantInfo
{
    GENERATED_BODY()

    // 참여자 식별자
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DialogueSystem")
    FName ParticipantID;

    // 참여자 색상 (UI 강조색 등)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DialogueSystem")
    FColor ParticipantColor = FColor::White;
};

USTRUCT(BlueprintType)
struct DIALOGUESYSTEMRUNTIME_API FDialogueParticipants
{
    GENERATED_BODY()

    // GameplayTag : 참여자 정보 맵
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DialogueSystem")
    TMap<FGameplayTag, FDialogueParticipantInfo> ParticipantInfoMap;

    // 지정 태그의 참여자 정보를 안전하게 반환. 없으면 false
    bool TryGetParticipantInfo(const FGameplayTag& InTag, FDialogueParticipantInfo& OutInfo) const
    {
        if (const FDialogueParticipantInfo* Found = ParticipantInfoMap.Find(InTag))
        {
            OutInfo = *Found;
            return true;
        }
        return false;
    }
};

USTRUCT(BlueprintType)
struct DIALOGUESYSTEMRUNTIME_API FDialogueChangeMessage
{
    GENERATED_BODY()

    FDialogueChangeMessage() = default;
    FDialogueChangeMessage(UDialogueData* InData, UDialogueNode* InNode)
        : DialogueData(InData), CurrentNode(InNode) {}

    // 현재 대화 데이터 에셋
    UPROPERTY(BlueprintReadOnly, Category = "DialogueSystem")
    TObjectPtr<UDialogueData> DialogueData = nullptr;

    // 현재 노드
    UPROPERTY(BlueprintReadOnly, Category = "DialogueSystem")
    TObjectPtr<UDialogueNode> CurrentNode = nullptr;
};

USTRUCT(BlueprintType)
struct DIALOGUESYSTEMRUNTIME_API FDialogueHistory
{
    GENERATED_BODY()

    // 이전에 방문한 노드들 (복제본)
    UPROPERTY(BlueprintReadOnly, Category = "DialogueSystem")
    TArray<TObjectPtr<UDialogueNode>> PreviousNodes;
};
