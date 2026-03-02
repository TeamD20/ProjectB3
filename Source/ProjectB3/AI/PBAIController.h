// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "PBAIController.generated.h"

// AI 캐릭터 제어를 담당하는 기반 컨트롤러. AI 공통 로직을 여기서 관리.
UCLASS()
class PROJECTB3_API APBAIController : public AAIController
{
	GENERATED_BODY()

public:
	APBAIController();
};
