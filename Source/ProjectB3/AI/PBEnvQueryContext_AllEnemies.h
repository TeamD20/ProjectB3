// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "PBEnvQueryContext_AllEnemies.generated.h"

// EQS Context: 현재 턴의 모든 적 액터를 제공한다.
// Clearinghouse의 CachedTargets 전체를 액터 배열로 반환.
// EQS_FindFallbackPosition 쿼리의 PBEnvQueryTest_LineOfSight에서
// "적 중 아무나 한 명이라도 보이는 위치" 필터링에 사용된다.
UCLASS()
class PROJECTB3_API UPBEnvQueryContext_AllEnemies : public UEnvQueryContext
{
	GENERATED_BODY()

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance,
	                            FEnvQueryContextData& ContextData) const override;
};
