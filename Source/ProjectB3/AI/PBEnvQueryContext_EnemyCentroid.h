// PBEnvQueryContext_EnemyCentroid.h
#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "PBEnvQueryContext_EnemyCentroid.generated.h"

// EQS Context: 적 무리의 중심 위치(Centroid)를 제공한다.
// Clearinghouse의 CachedTargets로부터 계산된 평균 위치.
// EQS_FindFallbackPosition 쿼리의 "Distance from Enemy Centroid" 테스트에서
// Context로 사용된다.
UCLASS()
class PROJECTB3_API UPBEnvQueryContext_EnemyCentroid : public UEnvQueryContext
{
	GENERATED_BODY()

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance,
	                            FEnvQueryContextData& ContextData) const override;
};
