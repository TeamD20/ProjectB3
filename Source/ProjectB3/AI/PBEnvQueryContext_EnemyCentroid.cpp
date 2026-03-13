// PBEnvQueryContext_EnemyCentroid.cpp

#include "PBEnvQueryContext_EnemyCentroid.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "PBUtilityClearinghouse.h"

void UPBEnvQueryContext_EnemyCentroid::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData) const
{
	// Querier(AI 폰)로부터 월드 → Clearinghouse 서브시스템 획득
	const AActor* QuerierActor = Cast<AActor>(QueryInstance.Owner.Get());
	if (!IsValid(QuerierActor))
	{
		return;
	}

	const UWorld* World = QuerierActor->GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const UPBUtilityClearinghouse* Clearinghouse =
		World->GetSubsystem<UPBUtilityClearinghouse>();
	if (!IsValid(Clearinghouse))
	{
		return;
	}

	// Clearinghouse에서 적 Centroid 계산하여 위치 Context로 제공
	const FVector Centroid = Clearinghouse->GetEnemyCentroid();
	if (Centroid.IsZero())
	{
		return;
	}

	UEnvQueryItemType_Point::SetContextHelper(ContextData, Centroid);
}
