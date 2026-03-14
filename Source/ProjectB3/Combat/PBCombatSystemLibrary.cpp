// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBCombatSystemLibrary.h"

#include "PBCombatManagerSubsystem.h"

UPBCombatManagerSubsystem* UPBCombatSystemLibrary::GetCombatManager(UObject* WorldContextObject)
{
	if (IsValid(WorldContextObject) && IsValid(WorldContextObject->GetWorld()))
	{
		return WorldContextObject->GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
	}
	return nullptr;
}

bool UPBCombatSystemLibrary::IsInCombat(UObject* WorldContextObject)
{
	if (UPBCombatManagerSubsystem* CombatManager = GetCombatManager(WorldContextObject))
	{
		return CombatManager->IsInCombat();
	}
	return false;
}
