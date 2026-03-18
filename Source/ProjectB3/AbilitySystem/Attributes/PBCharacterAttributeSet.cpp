// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCharacterAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"

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

	// 3. 파생 전투 어트리뷰트 초기화 (기본값. Infinite GE로 덮어씁.)
	InitHitBonus(0.0f);
	InitAttackModifier(0.0f);
	InitSpellSaveDCModifier(0.0f); 

	// 4. 숙련 보너스 초기화  (기본값. Infinite GE로 덮어씁.)
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

	UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(GetOwningAbilitySystemComponent());

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		// 메타 어트리뷰트 소비
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f);

		FGameplayTagContainer AssetTags;
		Data.EffectSpec.GetAllAssetTags(AssetTags);
		const bool bMiss = AssetTags.HasTag(PBGameplayTags::Combat_Result_Miss);

		if (!bMiss && Damage > 0.f)
		{
			SetHP(FMath::Max(0.f, GetHP() - Damage));
		}

		//GE 실행 결과 중계 (Miss이면 EffectiveValue = 0)
		if (IsValid(PBASC))
		{
			PBASC->NotifyGEExecuted(Data.EffectSpec, Data.EvaluatedData.Attribute, bMiss ? 0.f : Damage);
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

		// GE 실행 결과 중계
		if (IsValid(PBASC))
		{
			PBASC->NotifyGEExecuted(Data.EffectSpec, Data.EvaluatedData.Attribute, Heal);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHPAttribute())
	{
		SetHP(FMath::Clamp(GetHP(), 0.0f, GetMaxHP()));
	}
}


void UPBCharacterAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetHPAttribute())
	{
		if (NewValue < OldValue && FMath::IsNearlyEqual(NewValue,0.0f))
		{
			UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
			if (ASC && !ASC->HasMatchingGameplayTag(PBGameplayTags::Character_State_Dead))
			{
				ASC->AddLooseGameplayTag(PBGameplayTags::Character_State_Dead);
			}
		}
	}
}
