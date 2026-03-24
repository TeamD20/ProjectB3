// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBNavQueryFilter_IgnoreHazard.h"
#include "PBNavArea_Hazard.h"

UPBNavQueryFilter_IgnoreHazard::UPBNavQueryFilter_IgnoreHazard()
{
	// UPBNavArea_Hazard의 TravelCost를 기본값(1.0)으로 재정의하여
	// 플레이어 캐릭터가 위험 영역을 일반 영역처럼 통과하도록 한다.
	FNavigationFilterArea HazardOverride;
	HazardOverride.AreaClass = UPBNavArea_Hazard::StaticClass();
	HazardOverride.bOverrideTravelCost = true;
	HazardOverride.TravelCostOverride = 1.0f;
	Areas.Add(HazardOverride);
}
