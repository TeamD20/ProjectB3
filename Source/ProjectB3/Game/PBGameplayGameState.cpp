// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBGameplayGameState.h"

void APBGameplayGameState::NotifyPartyMemberListReady(const TArray<AActor*>& InPartyMembers)
{
	OnPartyMemberListReady.Broadcast(InPartyMembers);
}

void APBGameplayGameState::NotifyCombatStarted()
{
	OnCombatStarted.Broadcast();
}
