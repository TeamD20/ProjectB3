// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCharacterAttributeSet.h"
#include "GameplayEffectExtension.h"

UPBCharacterAttributeSet::UPBCharacterAttributeSet()
{
	// 1. 핵심 능력치 초기화 (기본값 10)
	InitStrength(10.0f);
	InitDexterity(10.0f);
	InitConstitution(10.0f);
	InitIntelligence(10.0f);

	// 2. 전투 상태 능력치 초기화
	InitHP(10.0f);
	InitMaxHP(10.0f);
	InitArmorClass(10.0f);
	InitInitiativeBonus(0.0f);

	// 3. 파생 전투 어트리뷰트 초기화 (변쾔 전 기본값. Infinite GE로 덮어씁.)
	InitHitBonus(0.0f);
	InitAttackModifier(0.0f);
	InitSpellSaveDC(8.0f); // D&D 5e 최소치: 8 + 0 + 0

	// 4. 숙련 보너스 초기화 (레벨 1 기준: +2)
	InitProficiencyBonus(2.0f);
}

void UPBCharacterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 상태 제약
	if (Attribute == GetMaxHPAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
	else if (Attribute == GetHPAttribute())
	{
		// HP는 0과 MaxHP 사이의 값을 가져야 함
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHP());
	}
}

void UPBCharacterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		// 메타 어트리뷰트 소비 후 HP에 반영
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f);

		if (Damage > 0.f)
		{
			SetHP(FMath::Max(0.f, GetHP() - Damage));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		const float Heal = GetIncomingHeal();
		SetIncomingHeal(0.f);

		if (Heal > 0.f)
		{
			SetHP(FMath::Min(GetMaxHP(), GetHP() + Heal));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHPAttribute())
	{
		SetHP(FMath::Clamp(GetHP(), 0.0f, GetMaxHP()));
	}
}
