// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "NativeGameplayTags.h"

// 프로젝트 전용 Native GameplayTag 선언 모음. 태그 추가 시 이 네임스페이스 안에 선언.
// UI 관련 태그는 UI/PBUITags에 분리.
namespace PBGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(System_Test);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_InCombat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Player);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Enemy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Neutral);

	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Innate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Class);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Weapon);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Armor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Accessory);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Status);
}
