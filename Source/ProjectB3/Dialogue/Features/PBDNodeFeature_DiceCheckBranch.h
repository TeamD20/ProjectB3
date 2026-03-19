// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogueFeatures/DNodeFeature_Branch.h"
#include "PBDNodeFeature_DiceCheckBranch.generated.h"

/**
 * 주사위 체크 + 성공/실패 분기 통합 Feature.
 * OnStartDialogueNode에서 ViewModel->ShowDiceRoll()을 호출하여 주사위 UI를 구동한다.
 * 분기는 성공=OptionId 0, 실패=OptionId 1 규칙을 따른다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Dice Check Branch"))
class PROJECTB3_API UPBDNodeFeature_DiceCheckBranch : public UDNodeFeature_Branch
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** ViewModel->ShowDiceRoll()을 호출하여 주사위 굴리기 UI를 구동 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;

    /*~ UDNodeFeature_Branch Interface ~*/
    /** OptionId 0=성공 노드, 1=실패 노드 반환 */
    virtual FName GetNextNodeId(const int32 OptionId) const override;

    /** Success/Failure 두 개 링크를 반환 */
    virtual TArray<FDialogueNodeLink> GetAllLinks() const override;

    /** 에디터에서 링크 업데이트 시 SuccessNodeId/FailureNodeId를 갱신 */
    virtual void UpdateLinks(const TArray<FDialogueNodeLink>& InLinks) override;

    /** 에디터 그래프에서 주사위 분기를 구분할 색상 반환 */
    virtual FLinearColor GetBranchColor() const override { return FLinearColor(0.9f, 0.7f, 0.1f); }

    /** 체크 정보 반환 */
    void GetDiceCheckInfo(FGameplayTag& OutSkillTag, int32& OutDC, bool& OutbAutoRoll) const;

public:
    // 체크 스킬 태그 (설득, 위협 등)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DiceCheck")
    FGameplayTag SkillCheckTag;

    // 난이도 등급
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DiceCheck")
    int32 DC = 10;

    // true면 노드 진입 시 자동 굴림, false면 플레이어 트리거
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DiceCheck")
    bool bAutoRoll = false;

    // 성공 시 이동할 노드 ID
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DiceCheck")
    FName SuccessNodeId;

    // 실패 시 이동할 노드 ID
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DiceCheck")
    FName FailureNodeId;
};
