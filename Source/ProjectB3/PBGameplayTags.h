// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "NativeGameplayTags.h"

// 프로젝트 전용 Native GameplayTag 선언 모음. 태그 추가 시 이 네임스페이스 안에 선언.
// UI 관련 태그는 UI/PBUITags에 분리.
namespace PBGameplayTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(System_Test);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Player); // 플레이어 관련 태그
	// 캐릭터
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class_Fighter); // 임시
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class_Ranger);  // 임시
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Class_Magician);  // 임시
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Identity_Varain); // 주인공 전사
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Identity_Faelis); // 주인공 궁수
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_Identity_Zadikal); // 주인공 법사
	
	// 상태 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Incapacitated); // 행동 불능
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Lootable); // 루팅 가능 상태 (사망/기절 등)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Moving);

	// 디버프 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Debuff_Burning);  // 화상
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Debuff_Poisoned); // 중독
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Debuff_Stunned);  // 기절
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Debuff_Sundered); // 방어도 약화
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Debuff_Baned);    // 내성 약화
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Debuff_Blinded);  // 명중률 약화

	// 버프 상태
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Buff_Blessed);   // 축복
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Buff_Shielded);  // 방어도 강화
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Buff_Inspired);  // 명중률 강화
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Character_State_Buff_Raging);    // 데미지 강화
	
	// 장비
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_LeftHand);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Equipment_Slot_RightHand);
	
	// 전투
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_State_InCombat);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Hit_Critical); // 치명타 명중 컨텍스트 태그

	// 전투 결과 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Result_Miss);         // 명중 실패
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Result_Save_Success); // 내성 성공 (데미지 반감)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Result_Save_Failed);  // 내성 실패 (데미지 전량)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Player);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Enemy);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Faction_Neutral);
	
	// 데미지 유형 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_DamageType_Melee);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_DamageType_Ranged);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_DamageType_Spell);
	
	// 어빌리티
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Common);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Class);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Innate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Weapon);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Armor);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Equipment_Accessory);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Source_Status);
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Active_Move);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive_HitReact);

	// 어빌리티 타입 태그 (행동 자원 소모 유형)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Action);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_BonusAction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Reaction);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Type_Free);

	// 어빌리티 분류 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Spell);

	// 어빌리티 효과 태그 (DFS 시퀀스 최적화용)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Effect_Displacement); // 넉백/밀치기 등 타겟 위치 변경

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

	// 아이템 이벤트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Item_UseConsumable); // 소비 아이템 사용 요청 이벤트

	// UI 이벤트 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_UI_FloatingText); // 플로팅 텍스트 위젯 표시 이벤트
	
	// Gameplay Cues
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Damage);   // 데미지 GameplayCue
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Heal);     // 힐 GameplayCue
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Explode);     // 폭발 GameplayCue

	// 버프 GameplayCue (부여 시 Burst VFX)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Buff_AC);  // 방어도 강화 버프
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Buff_ATK); // 명중률 강화 버프
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_Buff_DMG); // 데미지 강화 버프
	
	// 에셋 태그
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Bundle_Preload);
}
