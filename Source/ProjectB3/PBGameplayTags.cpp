// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayTags.h"

namespace PBGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(System_Test, "System.Test", "This is Test GameplayTag");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Class_Fighter, "Character.Class.Fighter", "근접 전투원"); // 임시
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Class_Ranger, "Character.Class.Ranger", "원거리 전투원");  // 임시
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_Class_Magician, "Character.Class.Magician", "마법 전투원");  // 임시
	// 상태 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Dead, "Character.State.Dead", "캐릭터 사망 상태");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Lootable, "Character.State.Lootable", "루팅 가능 상태. 사망/기절 등 조건 달성 시 부여");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Character_State_Incapacitated, "Character.State.Incapacitated", "행동 불능 상태");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_State_InCombat, "Combat.State.InCombat", "전투 중 상태 표시");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Hit_Critical, "Combat.Hit.Critical", "치명타 명중 — GE SourceTags로 전달해 ExecCalc/UI에서 참조");

	// 전투 결과 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Result_Miss,         "Combat.Result.Miss",         "명중 실패");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Result_Save_Success, "Combat.Result.Save.Success", "내성 성공");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Result_Save_Failed,  "Combat.Result.Save.Failed",  "내성 실패");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Player, "Combat.Faction.Player", "플레이어 진영");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Enemy, "Combat.Faction.Enemy", "적 진영");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Faction_Neutral, "Combat.Faction.Neutral", "중립 진영");

	// 장비
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot, "Equipment.Slot", "장비 슬롯들의 부모 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_LeftHand, "Equipment.Slot.LeftHand", "왼손 장비 슬롯 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Equipment_Slot_RightHand, "Equipment.Slot.RightHand", "오른손 장비 슬롯 태그");
	
	// 어빌리티
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Common, "Ability.Source.Common", "공통 기본 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Class, "Ability.Source.Class", "클래스 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Innate, "Ability.Source.Innate", "캐릭터 특수 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Equipment_Weapon, "Ability.Source.Equipment.Weapon", "무기 장비 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Equipment_Armor, "Ability.Source.Equipment.Armor", "방어구 장비 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Equipment_Accessory, "Ability.Source.Equipment.Accessory", "악세서리 장비 어빌리티 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Source_Status, "Ability.Source.Status", "상태 효과 어빌리티 출처");
	
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Active_Move, "Ability.Active.Move", "이동 어빌리티");

	// 어빌리티 타입 태그 (행동 자원 소모 유형)
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type, "Ability.Type", "어빌리티 타입 구분 태그");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Action, "Ability.Type.Action", "주행동 소모");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_BonusAction, "Ability.Type.BonusAction", "보조행동 소모");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Reaction, "Ability.Type.Reaction", "반응 행동 소모");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Type_Free, "Ability.Type.Free", "행동 자원 소모 없음");

	// 어빌리티 분류 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Spell, "Ability.Spell", "주문 어빌리티");

	// 어빌리티 발동 이벤트 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Activate, "Event.Ability.Activate", "어빌리티 발동 요청 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Confirm, "Event.Ability.Confirm", "타겟 확정 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Ability_Cancel, "Event.Ability.Cancel", "타겟 취소 이벤트");
	
	// 캐릭터 이벤트 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_Attack, "Event.Character.Attack", "공격 몽타주 내 공격 판정 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_Die, "Event.Character.Die", "캐릭터 사망 이벤트");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Character_HitReact, "Event.Character.HitReact", "캐릭터 피격 반응 이벤트");
	
	// 이동 이벤트 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Movement_MoveCommand, "Event.Movement.MoveCommand", "이동 명령 이벤트");

	// SetByCaller 어트리뷰트 초기화 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attribute_Strength, "SetByCaller.Attribute.Strength", "근력 초기값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attribute_Dexterity, "SetByCaller.Attribute.Dexterity", "민첩 초기값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attribute_Constitution, "SetByCaller.Attribute.Constitution", "건강 초기값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attribute_Intelligence, "SetByCaller.Attribute.Intelligence", "지능 초기값");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attribute_MaxHP, "SetByCaller.Attribute.MaxHP", "기본 최대 체력");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Attribute_ArmorClass, "SetByCaller.Attribute.ArmorClass", "기본 방어력");

	// SetByCaller 데미지 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage_DiceRoll, "SetByCaller.Damage.DiceRoll", "무기 데미지 주사위 결과 (치명타 포함)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage_AttackModifier, "SetByCaller.Damage.AttackModifier", "공격 수정치 (Str 또는 Dex)");

	// SetByCaller 회복 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Heal_Amount, "SetByCaller.Heal.Amount", "회복량");

	// UI 이벤트 태그
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_UI_FloatingText, "Event.UI.FloatingText", "플로팅 텍스트 위젯 표시 이벤트");
}