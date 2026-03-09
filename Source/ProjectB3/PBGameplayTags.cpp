// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayTags.h"

namespace PBGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(System_Test, "System.Test", "This is Test GameplayTag");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_InCombat, "Combat.State.InCombat", "전투 중 상태 표시");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Player, "Combat.Faction.Player", "플레이어 진영");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Enemy, "Combat.Faction.Enemy", "적 진영");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Neutral, "Combat.Faction.Neutral", "중립 진영");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Common, "Ability.Source.Common", "공통 기본 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Class, "Ability.Source.Class", "클래스 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Equipment_Weapon, "Ability.Source.Equipment.Weapon", "무기 장비 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Equipment_Armor, "Ability.Source.Equipment.Armor", "방어구 장비 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Equipment_Accessory, "Ability.Source.Equipment.Accessory", "악세서리 장비 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Status, "Ability.Source.Status", "상태 효과 어빌리티 출처");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Active_Move, "Ability.Active.Move", "이동 어빌리티");

	// 어빌리티 발동 이벤트 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Activate, "Event.Ability.Activate", "어빌리티 발동 요청 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Confirm, "Event.Ability.Confirm", "타겟 확정 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Cancel, "Event.Ability.Cancel", "타겟 취소 이벤트");

	// 이동 이벤트 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Movement_MoveCommand, "Event.Movement.MoveCommand", "이동 명령 이벤트");
}