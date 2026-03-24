// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "PBNavArea_Hazard.generated.h"

/**
 * 위험 영역용 NavArea.
 * 높은 EnterCost를 설정하여 AI 경로 탐색 시 자연스럽게 우회하도록 유도한다.
 * 완전 차단이 아닌 고비용이므로, 우회가 불가능하면 관통 이동도 허용된다.
 */
UCLASS()
class PROJECTB3_API UPBNavArea_Hazard : public UNavArea
{
	GENERATED_BODY()

public:
	UPBNavArea_Hazard();
};
