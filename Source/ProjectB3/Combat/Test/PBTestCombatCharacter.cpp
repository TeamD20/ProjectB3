// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTestCombatCharacter.h"

APBTestCombatCharacter::APBTestCombatCharacter()
{
}

int32 APBTestCombatCharacter::GetInitiativeModifier() const
{
	return TestInitiativeModifier;
}

bool APBTestCombatCharacter::HasInitiativeAdvantage() const
{
	return bTestHasAdvantage;
}

bool APBTestCombatCharacter::IsIncapacitated() const
{
	return bTestIsIncapacitated;
}

void APBTestCombatCharacter::SetIncapacitated(bool bNewIncapacitated)
{
	bTestIsIncapacitated = bNewIncapacitated;
}

bool APBTestCombatCharacter::CanReact() const
{
	return !bTestIsIncapacitated && bTestCanReact;
}

void APBTestCombatCharacter::SetCanReact(bool bNewCanReact)
{
	bTestCanReact = bNewCanReact;
}

void APBTestCombatCharacter::ResetTurnCallCounts()
{
	TurnBeginCount = 0;
	TurnActivatedCount = 0;
}

void APBTestCombatCharacter::OnTurnBegin()
{
	Super::OnTurnBegin();
	TurnBeginCount++;
}

void APBTestCombatCharacter::OnTurnActivated()
{
	Super::OnTurnActivated();
	TurnActivatedCount++;
}

void APBTestCombatCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if (NewController != nullptr)
	{
		UE_LOG(LogTemp,Warning,TEXT("PossesdBy %s"), *NewController->GetName());	
	}
}
