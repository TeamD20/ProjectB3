// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilityBlueprintLibrary.h"

AActor* UPBAbilityBlueprintLibrary::GetSingleTargetActor(const FPBAbilityTargetData& TargetData)
{
	return TargetData.GetSingleTargetActor();
}

TArray<AActor*> UPBAbilityBlueprintLibrary::GetAllTargetActors(const FPBAbilityTargetData& TargetData)
{
	return TargetData.GetAllTargetActors();
}

FVector UPBAbilityBlueprintLibrary::GetSingleTargetLocation(const FPBAbilityTargetData& TargetData)
{
	return TargetData.GetSingleTargetLocation();
}

TArray<FVector> UPBAbilityBlueprintLibrary::GetAllTargetLocations(const FPBAbilityTargetData& TargetData)
{
	return TArray<FVector>(TargetData.GetAllTargetLocations());
}

bool UPBAbilityBlueprintLibrary::HasTarget(const FPBAbilityTargetData& TargetData)
{
	return TargetData.HasTarget();
}

bool UPBAbilityBlueprintLibrary::IsTargetDataValid(const FPBAbilityTargetData& TargetData)
{
	return TargetData.IsValid();
}

TArray<AActor*> UPBAbilityBlueprintLibrary::GetAllHitActors(const FPBAbilityTargetData& TargetData)
{
	TArray<AActor*> Result;
	for (const TWeakObjectPtr<AActor>& Weak : TargetData.HitActors)
	{
		if (Weak.IsValid())
		{
			Result.Add(Weak.Get());
		}
	}
	return Result;
}
