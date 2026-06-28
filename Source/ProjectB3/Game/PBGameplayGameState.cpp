// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (파티 선택 상태와 턴 전투 이벤트 처리)


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
