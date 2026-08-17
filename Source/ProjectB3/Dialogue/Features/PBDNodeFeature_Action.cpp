// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (대화 노드 액션 실행 처리)

#include "PBDNodeFeature_Action.h"

#include "DialogueSystemTypes.h"

AActor* UPBDNodeFeature_Action::FindParticipantActor(const FDialogueSystemContext& InContext, const FGameplayTag& InParticipantTag) const
{
    // 1. ParticipantActors 맵에서 태그로 직접 조회
    if (AActor* Found = InContext.FindParticipantActor(InParticipantTag))
    {
        return Found;
    }

    // 2. Fallback: 대화 대상 NPC Actor 반환
    return InContext.TargetActor.Get();
}
