// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBGameplayStatics.h"
#include "Components/MeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"

void UPBGameplayStatics::GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// 현재 액터의 모든 MeshComponent 수집
	Actor->GetComponents<UMeshComponent>(OutMeshes, /*bIncludeFromChildActors=*/false);

	// ChildActorComponent를 통한 자식 액터 재귀 탐색
	TArray<UChildActorComponent*> ChildActorComps;
	Actor->GetComponents<UChildActorComponent>(ChildActorComps);
	for (UChildActorComponent* ChildActorComp : ChildActorComps)
	{
		if (!IsValid(ChildActorComp))
		{
			continue;
		}

		TArray<UMeshComponent*> ChildMeshes;
		GetAllMeshComponents(ChildActorComp->GetChildActor(), ChildMeshes);
		OutMeshes.Append(ChildMeshes);
	}

	// 부착된 액터 재귀 탐색
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors, /*bResetArray=*/true, /*bRecursivelyIncludeAttachedActors=*/false);
	for (AActor* AttachedActor : AttachedActors)
	{
		TArray<UMeshComponent*> AttachedMeshes;
		GetAllMeshComponents(AttachedActor, AttachedMeshes);
		OutMeshes.Append(AttachedMeshes);
	}
}

bool UPBGameplayStatics::SimpleMoveToLocation(AController* Controller, const FVector& GoalLocation, float AcceptanceRadius)
{
	if (!IsValid(Controller))
	{
		return false;
	}

	UWorld* World = Controller->GetWorld();
	if (!IsValid(World) || !IsValid(World->GetGameInstance()))
	{
		return false;
	}

	if (UPBEnvironmentSubsystem* EnvironmentSubsystem = World->GetGameInstance()->GetSubsystem<UPBEnvironmentSubsystem>())
	{
		return EnvironmentSubsystem->RequestMoveToLocation(Controller, GoalLocation, AcceptanceRadius, false);
	}

	return false;
}
