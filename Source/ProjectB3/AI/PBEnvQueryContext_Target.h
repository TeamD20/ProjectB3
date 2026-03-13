// PBEnvQueryContext_Target.h
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "PBEnvQueryContext_Target.generated.h"

// EQS Context: 현재 AI가 공격하려는 타겟 액터의 위치를 제공한다.
// Clearinghouse의 EQSTargetActor에서 읽어오며,
// EQS_FindAttackPosition 쿼리의 "Distance to Target", "Trace to Target" 등
// 테스트에서 Context로 사용된다.
UCLASS()
class PROJECTB3_API UPBEnvQueryContext_Target : public UEnvQueryContext
{
	GENERATED_BODY()

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance,
	                            FEnvQueryContextData& ContextData) const override;
};
