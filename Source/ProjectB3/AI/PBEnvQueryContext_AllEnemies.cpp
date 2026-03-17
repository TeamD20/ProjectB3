// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvQueryContext_AllEnemies.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "PBUtilityClearinghouse.h"

void UPBEnvQueryContext_AllEnemies::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData) const
{
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

	// CachedTargets에서 유효한 적 액터를 수집하여 Context로 제공
	TArray<AActor*> ValidEnemies;
	for (const TWeakObjectPtr<AActor>& TargetWeak : Clearinghouse->GetCachedTargets())
	{
		if (AActor* Target = TargetWeak.Get())
		{
			ValidEnemies.Add(Target);
		}
	}

	if (ValidEnemies.Num() > 0)
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, ValidEnemies);
	}
}
