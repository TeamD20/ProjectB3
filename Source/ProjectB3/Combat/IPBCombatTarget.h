// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IPBCombatTarget.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPBCombatTarget : public UInterface
{
	GENERATED_BODY()
};

/** 전투 타겟팅 대상이 될 수 있는 액터가 구현하는 인터페이스 */
class PROJECTB3_API IPBCombatTarget
{
	GENERATED_BODY()

public:
	// 타겟팅 시 조준점으로 사용할 위치 반환 (예: 캐릭터 흉부 등)
	virtual FVector GetCombatTargetLocation() const = 0;
};
