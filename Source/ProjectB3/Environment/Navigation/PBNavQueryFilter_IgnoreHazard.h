// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "PBNavQueryFilter_IgnoreHazard.generated.h"

/**
 * 플레이어 캐릭터 전용 내비게이션 쿼리 필터.
 * UPBNavArea_Hazard의 이동 비용을 기본값(1.0)으로 오버라이드하여
 * 위험 영역이 플레이어 경로 탐색에 영향을 주지 않도록 한다.
 */
UCLASS()
class PROJECTB3_API UPBNavQueryFilter_IgnoreHazard : public UNavigationQueryFilter
{
	GENERATED_BODY()

public:
	UPBNavQueryFilter_IgnoreHazard();
};
