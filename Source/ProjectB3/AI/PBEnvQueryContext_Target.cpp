// PBEnvQueryContext_Target.cpp

#include "PBEnvQueryContext_Target.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "PBUtilityClearinghouse.h"

void UPBEnvQueryContext_Target::ProvideContext(
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

	// Clearinghouse에 세팅된 현재 EQS 타겟 액터를 Context로 제공
	AActor* TargetActor = Clearinghouse->GetEQSTargetActor();
	if (!IsValid(TargetActor))
	{
		return;
	}

	UEnvQueryItemType_Actor::SetContextHelper(ContextData, TargetActor);
}
