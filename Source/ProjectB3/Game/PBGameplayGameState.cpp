// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBGameplayGameState.h"

#include "PBGameplayGameMode.h"
#include "ProjectB3/Player/PBGameplayPlayerController.h"

void APBGameplayGameState::NotifyPartyMemberListReady(const TArray<AActor*>& InPartyMembers)
{
	OnPartyMemberListReady.Broadcast(InPartyMembers);
}

void APBGameplayGameState::NotifyCombatStarted()
{
	OnCombatStarted.Broadcast();
}

void APBGameplayGameState::NotifyPartyMemberDeath(const AActor* InPartyMember)
{
	if (APBGameplayGameMode* GM = GetWorld()->GetAuthGameMode<APBGameplayGameMode>())
	{
		if (GM->CheckGameOver())
		{
			if (APBGameplayPlayerController* PC = GetWorld()->GetFirstPlayerController<APBGameplayPlayerController>())
			{
				PC->OnGameOver();
			}
		}
	}
}
