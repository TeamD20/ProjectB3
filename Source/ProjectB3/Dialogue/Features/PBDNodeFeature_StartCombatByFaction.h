// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBDNodeFeature_Action.h"
#include "PBDNodeFeature_StartCombatByFaction.generated.h"

struct FDialogueSystemContext;

/**
 * 전투 시작 Feature.
 * 대화 대상 액터의 FactionTag와 동일한 반경 내 APBCharacterBase들을 수집하고,
 * 플레이어 파티원과 합쳐 전투를 시작한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Start Combat By Faction"))
class PROJECTB3_API UPBDNodeFeature_StartCombatByFaction : public UPBDNodeFeature_Action
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** 주변 동일 진영 + 파티원을 전투원으로 수집하여 전투를 시작 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;

public:
    // 전투원 탐색 반경 (cm)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
    float SearchRadius = 3000.0f;

    // true면 FactionTag 완전 일치, false면 상위 태그 포함 매칭
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    bool bMatchFactionExactly = true;

    // true면 중심 액터(대화 대상)도 전투원에 포함
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
    bool bIncludeTargetActor = true;

protected:
    /** 중심 액터의 진영과 일치하는 주변 APBCharacterBase를 수집 */
    void CollectNearbySameFactionCombatants(AActor* InCenterActor, TArray<AActor*>& OutCombatants) const;

    /** 플레이어 파티원을 수집 */
    void CollectPlayerPartyMembers(const FDialogueSystemContext& InDialogueContext, TArray<AActor*>& OutCombatants) const;

    /** 전투 참가자 배열에 중복 없이 액터를 추가 */
    void AddUniqueCombatant(AActor* InActor, TArray<AActor*>& OutCombatants) const;

    /** 두 진영 태그가 일치하는지 확인 */
    bool IsFactionMatched(const FGameplayTag& InLhs, const FGameplayTag& InRhs) const;
};
