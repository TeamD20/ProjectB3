// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnPortraitViewModel.h"

void UPBTurnPortraitViewModel::InitializeTurnPortrait(const FPBTurnOrderEntry& InEntry)
{
	DisplayName = InEntry.DisplayName;
	Portrait = InEntry.Portrait;
	bIsAlly = InEntry.bIsAlly;
	bIsCurrentTurn = false;
	bIsDead = false;
	HealthPercent = InEntry.InitialHealthPercent;

	OnDisplayNameChanged.Broadcast(DisplayName);
	OnPortraitChanged.Broadcast(Portrait);
	OnHPPercentValueChanged.Broadcast(HealthPercent);
}

void UPBTurnPortraitViewModel::SetIsCurrentTurn(bool bInIsCurrentTurn)
{
	if (bIsCurrentTurn != bInIsCurrentTurn)
	{
		bIsCurrentTurn = bInIsCurrentTurn;
		OnCurrentTurnChanged.Broadcast(bIsCurrentTurn);
	}
}

void UPBTurnPortraitViewModel::SetIsDead(bool bInIsDead)
{
	if (bIsDead != bInIsDead)
	{
		bIsDead = bInIsDead;
		OnDeathStateChanged.Broadcast(bIsDead);
	}
}

void UPBTurnPortraitViewModel::SetHealthPercent(float InHealthPercent)
{
	if (HealthPercent != InHealthPercent)
	{
		HealthPercent = InHealthPercent;
		OnHPPercentValueChanged.Broadcast(HealthPercent);
	}
}
