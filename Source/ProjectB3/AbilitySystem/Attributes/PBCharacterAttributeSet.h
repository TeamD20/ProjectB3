// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PBCharacterAttributeSet.generated.h"

// 매크로를 이용해 속성의 Get, Set, Init 헬퍼 함수를 자동 생성합니다.
#ifndef ATTRIBUTE_ACCESSORS
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
#endif

/**
 * 발더스게이트3 스타일의 핵심 능력치 및 전투 상태 능력치 AttributeSet.
 */
UCLASS()
class PROJECTB3_API UPBCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	/** 기본 생성자 */
	UPBCharacterAttributeSet();
	
	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
public:
	// 근력 (Strength): 근접 공격 명중/피해
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Base")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, Strength)

	// 민첩 (Dexterity): 원거리 무기 명중/피해, AC(방어력) 보너스, 우선권(Initiative).
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Base")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, Dexterity)

	// 건강 (Constitution): 최대 체력 증가, 집중 유지 내성 굴림.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Base")
	FGameplayAttributeData Constitution;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, Constitution)

	// 지능 (Intelligence): 주문 시전 능력, 지식 관련.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Base")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, Intelligence)

	// 현재 체력 (HP)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData HP;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, HP)

	// 최대 체력 (MaxHP)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData MaxHP;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, MaxHP)

	// 방어력 (Armor Class, AC): 적의 공격이 명중하기 위해 넘어야 하는 수치.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData ArmorClass;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, ArmorClass)

	// 우선권 보너스 (Initiative Bonus): 전투 시작 시 턴 순서를 결정하는 가중치.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData InitiativeBonus;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, InitiativeBonus)

	// ==== 파생 전투 어트리뷰트 (Infinite GE + MMC로 갱신) ====

	// 명중 수정치 (HitBonus): BonusAttributeOverride 능력치 수정치.
	// 명중 굴림: d20 + HitBonus + ProficiencyBonus vs 대상 AC.
	// DiceSpec.BonusAttributeOverride 미지정 시 MakeDamageEffectSpec이 이 값을 폴백으로 사용.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData HitBonus;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, HitBonus)

	// 데미지 수정치 (AttackModifier): AttackModifierAttributeOverride 능력치 수정치.
	// 데미지 굴림: 주사위 합산 + AttackModifier.
	// DiceSpec.AttackModifierAttributeOverride 미지정 시 MakeDamageEffectSpec이 이 값을 폴백으로 사용.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData AttackModifier;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, AttackModifier)

	// 주문 난이도 수정치: 8 + ProficiencyBonus + SpellSaveDCModifier.
	// DiceSpec.AttackModifierAttributeOverride 미지정 시 MakeDamageEffectSpec이 이 값을 폴백으로 사용.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData SpellSaveDCModifier;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, SpellSaveDCModifier)

	// ==== 메타 어트리뷰트 (임시 전달용, GE 실행 후 자동 초기화) ====

	// 받을 데미지. ExecCalc에서 계산 후 PostGameplayEffectExecute에서 HP에 반영.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, IncomingDamage)

	// 받을 회복량. 힐 GE에서 설정 후 PostGameplayEffectExecute에서 HP에 반영.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingHeal;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, IncomingHeal)

	// ==== 숙련 보너스 ====

	// 숙련 보너스: 레벨 기반 공격 굴림·내성 굴림 보너스.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Proficiency")
	FGameplayAttributeData ProficiencyBonus;
	ATTRIBUTE_ACCESSORS(UPBCharacterAttributeSet, ProficiencyBonus)
};
