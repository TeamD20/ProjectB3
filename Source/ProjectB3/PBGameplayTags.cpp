// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayTags.h"

namespace PBGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(System_Test, "System.Test", "This is Test GameplayTag");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_InCombat, "Combat.State.InCombat", "전투 중 상태 표시");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Player, "Combat.Faction.Player", "플레이어 진영");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Enemy, "Combat.Faction.Enemy", "적 진영");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Neutral, "Combat.Faction.Neutral", "중립 진영");
}