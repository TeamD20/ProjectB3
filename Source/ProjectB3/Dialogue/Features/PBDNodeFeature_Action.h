// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogueFeatures/DNodeFeature.h"
#include "PBDNodeFeature_Action.generated.h"

/**
 * 연출 액션 Feature 추상 기반 클래스.
 * 카메라 전환, 애니메이션, 사운드 등 연출을 OnStartDialogueNode에서 직접 실행한다.
 * ViewModel을 통하지 않고 월드에 직접 작용한다.
 */
UCLASS(Abstract, BlueprintType)
class PROJECTB3_API UPBDNodeFeature_Action : public UDNodeFeature
{
    GENERATED_BODY()

protected:
    /** Context에서 지정 ParticipantTag에 해당하는 Actor를 반환 */
    AActor* FindParticipantActor(const FDialogueSystemContext& InContext, const FGameplayTag& InParticipantTag) const;
};
