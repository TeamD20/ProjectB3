// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogueSystemTypes.h"
#include "ProjectB3/Interaction/PBInteractionAction.h"
#include "PBInteraction_DialogueAction.generated.h"

class UDialogueData;
class UPBDialogueManagerComponent;
class UPBInteractorComponent;

/**
 * 대화 상호작용 행동.
 * NPC 클릭 시 DialogueManagerComponent에 StartDialogue를 요청하고,
 * 대화가 종료되면 InteractorComponent에 EndActiveInteraction을 알린다.
 */
UCLASS()
class PROJECTB3_API UPBInteraction_DialogueAction : public UPBInteractionAction
{
    GENERATED_BODY()

public:
    UPBInteraction_DialogueAction();

    /*~ UPBInteractionAction Interface ~*/
    /** 전투 중이 아닌지, 이미 대화 중이 아닌지 확인 */
    virtual bool CanInteract_Implementation(AActor* Interactor) const override;

    /** DialogueManagerComponent에 StartDialogue 요청 및 종료 델리게이트 바인딩 */
    virtual void Execute_Implementation(AActor* Interactor) override;

    /** 거리 초과 등 외부 종료 시 대화 강제 종료 */
    virtual void EndInteraction_Implementation() override;

public:
    // 이 NPC에 할당된 대화 데이터 에셋
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
    TObjectPtr<UDialogueData> DialogueDataAsset;

private:
    /** 대화 정상 종료 시 InteractorComponent에 EndActiveInteraction 요청 */
    UFUNCTION()
    void HandleDialogueEnded(FDialogueChangeMessage DialogueChangeMessage);

private:
    // 캐시된 DialogueManagerComponent (약참조)
    TWeakObjectPtr<UPBDialogueManagerComponent> CachedDialogueManager;

    // 캐시된 InteractorComponent (종료 요청용)
    TWeakObjectPtr<UPBInteractorComponent> CachedInteractorComponent;
};
