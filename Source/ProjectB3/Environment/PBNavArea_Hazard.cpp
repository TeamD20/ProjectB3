// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBNavArea_Hazard.h"

UPBNavArea_Hazard::UPBNavArea_Hazard()
{
	// 기본 NavArea의 DefaultCost는 1.0.
	// Hazard 영역은 높은 비용을 부여하여 AI가 우회를 우선시하도록 한다.
	// 우회 경로가 없으면 관통도 가능 (Null이 아닌 이상 완전 차단되지 않음).
	DefaultCost = 10.f;
}
