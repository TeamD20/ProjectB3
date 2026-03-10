// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCharacterBase.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySetData.h"

APBCharacterBase::APBCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// ASC 생성
	AbilitySystemComponent = CreateDefaultSubobject<UPBAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// 턴 자원 AttributeSet 생성 (ASC에 자동 등록)
	TurnResourceAttributeSet = CreateDefaultSubobject<UPBTurnResourceAttributeSet>(TEXT("TurnResourceAttributeSet"));
}

UAbilitySystemComponent* APBCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APBCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// InitAbilityActorInfo 이후 어빌리티 부여
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		GrantInitialAbilities();
	}
}

void APBCharacterBase::GrantInitialAbilities()
{
	if (IsValid(CommonAbilitySet))
	{
		AbilitySystemComponent->GrantAbilitiesFromData(
			PBGameplayTags::Ability_Source_Common, CommonAbilitySet);
	}

	if (IsValid(ClassAbilitySet))
	{
		// TODO: 캐릭터 레벨 시스템 연동 시 실제 레벨 전달
		AbilitySystemComponent->GrantAbilitiesFromData(
			PBGameplayTags::Ability_Source_Class, ClassAbilitySet, 1);
	}
}

int32 APBCharacterBase::GetInitiativeModifier() const
{
	// 하위 클래스에서 DEX 수정치 기반으로 override
	return 0;
}

bool APBCharacterBase::HasInitiativeAdvantage() const
{
	return false;
}

void APBCharacterBase::OnCombatBegin()
{
	bIsInCombat = true;
}

void APBCharacterBase::OnCombatEnd()
{
	bIsInCombat = false;
}

void APBCharacterBase::OnRoundBegin()
{
	// Reaction 리필
	if (IsValid(AbilitySystemComponent) && IsValid(TurnResourceAttributeSet))
	{
		AbilitySystemComponent->SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetReactionAttribute(), 1.0f);
	}
}

void APBCharacterBase::OnTurnBegin()
{
	// Action, BonusAction, Movement 리셋
	if (IsValid(AbilitySystemComponent) && IsValid(TurnResourceAttributeSet))
	{
		AbilitySystemComponent->SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetActionAttribute(), 1.0f);
		AbilitySystemComponent->SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetBonusActionAttribute(), 1.0f);
		AbilitySystemComponent->SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetMovementAttribute(), GetBaseMovementSpeed());
	}
}

void APBCharacterBase::OnTurnActivated()
{
	// 기본 구현: 별도 처리 없음 (하위 클래스에서 override)
}

void APBCharacterBase::OnTurnEnd()
{
	// 기본 구현: 별도 처리 없음
}

bool APBCharacterBase::CanReact() const
{
	if (IsIncapacitated())
	{
		return false;
	}

	if (IsValid(AbilitySystemComponent) && IsValid(TurnResourceAttributeSet))
	{
		return TurnResourceAttributeSet->GetReaction() > 0.0f;
	}

	return false;
}

void APBCharacterBase::OnReactionOpportunity(const FPBReactionContext& Context)
{
	// 기본 구현: 아무 것도 하지 않음 (하위 클래스에서 override)
}

void APBCharacterBase::OnActionInterrupted()
{
	// 기본 구현: 아무 것도 하지 않음 (하위 클래스에서 override)
}

bool APBCharacterBase::IsIncapacitated() const
{
	// 기본 구현: 하위 클래스에서 상태이상 시스템 기반으로 override
	return false;
}

void APBCharacterBase::SetCombatIdentity(const FPBCombatIdentity& InIdentity)
{
	CombatIdentity = InIdentity;
}

FGameplayTag APBCharacterBase::GetFactionTag() const
{
	if (CombatIdentity.FactionTag.IsValid())
	{
		return CombatIdentity.FactionTag;
	}
	return PBGameplayTags::Combat_Faction_Neutral;
}

float APBCharacterBase::GetBaseMovementSpeed() const
{
	return 30.0f;
}

FText APBCharacterBase::GetCombatDisplayName() const
{
	if (!CombatIdentity.DisplayName.IsEmpty())
	{
		return CombatIdentity.DisplayName;
	}
	return FText::FromString(GetName());
}

TSoftObjectPtr<UTexture2D> APBCharacterBase::GetCombatPortrait() const
{
	return CombatIdentity.Portrait;
}
