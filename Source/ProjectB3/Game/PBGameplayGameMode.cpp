// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayGameMode.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"

void APBGameplayGameMode::InitiateCombat(const TArray<AActor*>& Combatants)
{
	if (UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		CombatManager->StartCombat(Combatants);
	}
}
