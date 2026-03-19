// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogueFeatures/DNodeFeature_DefaultLinkBranch.h"
#include "PBDNodeFeature_DefaultLinkBranch.generated.h"

/**
 * 단일 링크 분기 Feature.
 * 표시 전환 없이 단순 진행을 담당한다.
 * 같은 노드에 Text Feature가 있으면 그 Feature가 표시를 담당한다.
 */
UCLASS(BlueprintType, meta = (DisplayName = "PB Dialogue Default Link"))
class PROJECTB3_API UPBDNodeFeature_DefaultLinkBranch : public UDNodeFeature_DefaultLinkBranch
{
    GENERATED_BODY()
};
