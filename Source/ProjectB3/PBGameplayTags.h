// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "NativeGameplayTags.h"

// 프로젝트 전용 Native GameplayTag 선언 모음. 태그 추가 시 이 네임스페이스 안에 선언.
// UI 관련 태그는 UI/PBUITags에 분리.
namespace PBGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(System_Test);
	// 캐릭터
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class_Fighter); // 임시
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class_Ranger);  // 임시
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class_Magician);  // 임시
	
	// 상태 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Dead);
	
	// 장비
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_LeftHand);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_RightHand);
	
	// 전투
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_InCombat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Hit_Critical); // 치명타 명중 컨텍스트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Player);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Enemy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Neutral);
	
	// 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Common);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Class);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Weapon);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Armor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Accessory);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Status);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Active_Move);

	// 어빌리티 타입 태그 (행동 자원 소모 유형)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Action);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_BonusAction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Reaction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Free);

	// 어빌리티 분류 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Spell);

	// 어빌리티 발동 이벤트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Activate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Confirm);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ability_Cancel);
	
	// 캐릭터 이벤트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Character_Attack);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Character_Die);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Character_HitReact);

	// 이동 이벤트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Movement_MoveCommand);

	// SetByCaller 어트리뷰트 초기화 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Attribute_Strength);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Attribute_Dexterity);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Attribute_Constitution);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Attribute_Intelligence);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Attribute_MaxHP);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Attribute_ArmorClass);

	// SetByCaller 데미지 태그 (어빌리티에서 주사위·수정치를 계산 후 GE에 전달)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_DiceRoll);       // 무기 데미지 주사위 결과 (치명타 포함)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_AttackModifier);  // 공격 수정치 (Str 또는 Dex)

	// SetByCaller 회복 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Heal_Amount); // 회복량
}
