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
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Common);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Class);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Weapon);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Armor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Accessory);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Status);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Active_Move);

	// 어빌리티 발동 이벤트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Activate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Confirm);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Cancel);

	// 이동 이벤트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Movement_MoveCommand);
}
