// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBDNodeFeature_Action.h"
#include "PBDNodeFeature_FaceTarget.generated.h"

/**
 * 참여자 시선 전환 Feature.
 * OnStartDialogueNode에서 Subject 참여자가 Target 참여자를 부드럽게 바라보도록 Yaw 회전시킨다.
 * OnEndDialogueNode에서 타이머를 정리한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Face Target"))
class PROJECTB3_API UPBDNodeFeature_FaceTarget : public UPBDNodeFeature_Action
{
    GENERATED_BODY()

public:
    /*~ UDNodeFeature Interface ~*/
    /** Subject 참여자의 부드러운 Yaw 회전 시작 */
    virtual void OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;
    /** 진행 중인 회전 타이머 정리 */
    virtual void OnEndDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext) override;

public:
    // 회전할 참여자 태그 (회전의 주체)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FaceTarget")
    FGameplayTag SubjectParticipantTag;

    // 바라볼 대상 참여자 태그
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FaceTarget")
    FGameplayTag TargetParticipantTag;

    // 회전 완료까지 소요 시간 (초)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FaceTarget")
    float Duration = 0.5f;

private:
    /** 매 타이머 틱마다 Yaw 보간 수행 */
    void TickRotation();

    /** 진행 중인 회전 타이머 핸들 */
    FTimerHandle RotationTimerHandle;

    // 회전 주체 Actor (약참조)
    TWeakObjectPtr<AActor> SubjectActorRef;

    // 회전 시작 시각 (FPlatformTime::Seconds)
    double StartTime = 0.0;

    // 시작 Yaw (도)
    float StartYaw = 0.f;

    // 목표 Yaw (도)
    float TargetYaw = 0.f;
};
