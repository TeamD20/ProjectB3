// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "PBAbilitySystemComponent.h"
#include "Attributes/PBCharacterAttributeSet.h"
#include "Data/PBAbilitySetData.h"
#include "Data/PBAbilitySystemRegistry.h"
#include "ProjectB3/Game/PBGameInstance.h"

AActor* UPBAbilitySystemLibrary::GetSingleTargetActor(const FPBAbilityTargetData& TargetData)
{
	return TargetData.GetSingleTargetActor();
}

TArray<AActor*> UPBAbilitySystemLibrary::GetAllTargetActors(const FPBAbilityTargetData& TargetData)
{
	return TargetData.GetAllTargetActors();
}

FVector UPBAbilitySystemLibrary::GetSingleTargetLocation(const FPBAbilityTargetData& TargetData)
{
	return TargetData.GetSingleTargetLocation();
}

TArray<FVector> UPBAbilitySystemLibrary::GetAllTargetLocations(const FPBAbilityTargetData& TargetData)
{
	return TArray<FVector>(TargetData.GetAllTargetLocations());
}

bool UPBAbilitySystemLibrary::HasTarget(const FPBAbilityTargetData& TargetData)
{
	return TargetData.HasTarget();
}

bool UPBAbilitySystemLibrary::IsTargetDataValid(const FPBAbilityTargetData& TargetData)
{
	return TargetData.IsValid();
}

TArray<AActor*> UPBAbilitySystemLibrary::GetAllHitActors(const FPBAbilityTargetData& TargetData)
{
	TArray<AActor*> Result;
	for (const TWeakObjectPtr<AActor>& Weak : TargetData.HitActors)
	{
		if (Weak.IsValid())
		{
			Result.Add(Weak.Get());
		}
	}
	return Result;
}

FPBHitRollResult UPBAbilitySystemLibrary::RollHit(int32 HitBonus, int32 TargetAC)
{
	FPBHitRollResult Result;
	Result.Roll = FMath::RandRange(1, 20);

	// Natural 1: 자동 실패
	if (Result.Roll == 1)
	{
		Result.bHit = false;
		Result.bCritical = false;
		return Result;
	}

	// Natural 20: 치명타 (자동 명중)
	if (Result.Roll == 20)
	{
		Result.bHit = true;
		Result.bCritical = true;
		return Result;
	}

	// 일반 판정: 굴림 + 명중 보너스 >= AC
	Result.bHit = (Result.Roll + HitBonus) >= TargetAC;
	Result.bCritical = false;
	return Result;
}

FPBDamageRollResult UPBAbilitySystemLibrary::RollDamage(int32 DiceCount, int32 DiceFaces, float AttackModifier, bool bCritical)
{
	FPBDamageRollResult Result;
	Result.AttackModifier = AttackModifier;

	// 치명타: 주사위 수 2배 (수정치는 그대로)
	const int32 ActualDiceCount = bCritical ? DiceCount * 2 : DiceCount;

	float DiceTotal = 0.f;
	for (int32 i = 0; i < ActualDiceCount; ++i)
	{
		DiceTotal += static_cast<float>(FMath::RandRange(1, DiceFaces));
	}
	Result.DiceRoll = DiceTotal;
	return Result;
}

FPBSavingThrowResult UPBAbilitySystemLibrary::RollSavingThrow(int32 SaveBonus, int32 SpellSaveDC)
{
	FPBSavingThrowResult Result;
	Result.Roll = FMath::RandRange(1, 20);
	// D&D 5e: 내성 굴림에는 Natural 1/20 자동 실패/성공 없음
	Result.bSucceeded = (Result.Roll + SaveBonus) >= SpellSaveDC;
	return Result;
}

float UPBAbilitySystemLibrary::CalcFinalDamage(float DiceRoll, float AttackModifier,
	const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& TargetTags)
{
	// TODO: SourceTags/TargetTags 기반 저항·취약 보정 추가 예정
	// (예: TargetTags.HasTag(Combat.Condition.Resistant) → *0.5, Vulnerable → *2)
	// 소수점 내림 적용
	return FMath::FloorToFloat(FMath::Max(0.f, DiceRoll + AttackModifier));
}

float UPBAbilitySystemLibrary::CalcExpectedDamage(int32 DiceCount, int32 DiceFaces, float AttackModifier,
	const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& TargetTags)
{
	// 평균 주사위 결과: 주사위 수 × (1 + 면 수) / 2
	const float AverageDiceRoll = DiceCount * (1.f + static_cast<float>(DiceFaces)) / 2.f;
	return CalcFinalDamage(AverageDiceRoll, AttackModifier, SourceTags, TargetTags);
}

float UPBAbilitySystemLibrary::CalcExpectedAttackDamage(
	int32 DiceCount, int32 DiceFaces, float AttackModifier,
	int32 HitBonus, int32 TargetAC,
	const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& TargetTags)
{
	// d20 굴림 분포: 1=자동실패, 20=치명타, 2~19=일반 판정
	// 일반 명중 최소 굴림값
	const int32 MinHitRoll = FMath::Clamp(TargetAC - HitBonus, 2, 20);
	// 2~19 범위에서 명중하는 값의 수 (19를 넘으면 치명타이므로 제외)
	const int32 NormalHitCount = FMath::Max(0, 19 - MinHitRoll + 1);

	const float PNormalHit = NormalHitCount / 20.f;
	const float PCrit      = 1.f / 20.f;

	// 치명타: 주사위 수 2배
	const float AvgNormal = CalcExpectedDamage(DiceCount,     DiceFaces, AttackModifier, SourceTags, TargetTags);
	const float AvgCrit   = CalcExpectedDamage(DiceCount * 2, DiceFaces, AttackModifier, SourceTags, TargetTags);

	return PNormalHit * AvgNormal + PCrit * AvgCrit;
}

float UPBAbilitySystemLibrary::CalcExpectedSavingThrowDamage(
	int32 DiceCount, int32 DiceFaces, float AttackModifier,
	int32 SaveBonus, int32 SpellSaveDC,
	const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& TargetTags)
{
	// 실패 조건: roll + SaveBonus < SpellSaveDC → roll < SpellSaveDC - SaveBonus
	const int32 FailCount = FMath::Clamp(SpellSaveDC - SaveBonus - 1, 0, 20);
	const float PFail = FailCount / 20.f;

	// 성공 시 절반 데미지: 기댓값 = avgDmg * (0.5 + 0.5 * P(실패))
	const float AvgDamage = CalcExpectedDamage(DiceCount, DiceFaces, AttackModifier, SourceTags, TargetTags);
	return AvgDamage * (0.5f + 0.5f * PFail);
}

int32 UPBAbilitySystemLibrary::CalcAbilityModifier(float AbilityScore)
{
	// 능력치 수정치: floor((능력치 - 10) / 2)
	return FMath::FloorToInt32((AbilityScore - 10.f) / 2.f);
}

int32 UPBAbilitySystemLibrary::GetAttackModifier(const UAbilitySystemComponent* ASC, const FGameplayAttribute& KeyAttributeOverride)
{
	return GetAbilityModifierValue(ASC,UPBCharacterAttributeSet::GetAttackModifierAttribute(), KeyAttributeOverride );
}

int32 UPBAbilitySystemLibrary::CalcSpellSaveDC(const UAbilitySystemComponent* SourceASC,
	const FGameplayAttribute& KeyAttributeOverride)
{
	if (!IsValid(SourceASC))
	{
		return 0;
	}

	// KeyAttributeOverride 지정 시: 8 + ProfBonus + 해당 능력치 수정치로 직접 계산
	if (KeyAttributeOverride.IsValid())
	{
		bool bFound = false;
		const float AttrValue = SourceASC->GetGameplayAttributeValue(KeyAttributeOverride, bFound);
		const int32 AttrModifier = bFound ? CalcAbilityModifier(AttrValue) : 0;
		return 8 + GetProficiencyBonus(SourceASC) + AttrModifier;
	}

	// 미지정 시: AttributeSet의 SpellSaveDC 어트리뷰트 값 직접 반환 (Infinite GE로 갱신된 값)
	bool bFound = false;
	const float Value = SourceASC->GetGameplayAttributeValue(
		UPBCharacterAttributeSet::GetSpellSaveDCAttribute(), bFound);
	return bFound ? static_cast<int32>(Value) : 0;
}

int32 UPBAbilitySystemLibrary::GetHitBonus(const UAbilitySystemComponent* ASC, const FGameplayAttribute& KeyAttributeOverride)
{
	return GetAbilityModifierValue(ASC,UPBCharacterAttributeSet::GetHitBonusAttribute(), KeyAttributeOverride );
}


int32 UPBAbilitySystemLibrary::GetProficiencyBonus(const UAbilitySystemComponent* ASC)
{
	if (!IsValid(ASC))
	{
		return 0;
	}

	bool bFound = false;
	const float Value = ASC->GetGameplayAttributeValue(
		UPBCharacterAttributeSet::GetProficiencyBonusAttribute(), bFound);
	return bFound ? static_cast<int32>(Value) : 0;
}


float UPBAbilitySystemLibrary::CalcMaxHPBonus(float Constitution, int32 Level)
{
	// 레벨당 Con Modifier만큼 MaxHP 보너스 부여
	const int32 ConModifier = CalcAbilityModifier(Constitution);
	return static_cast<float>(ConModifier * Level);
}

float UPBAbilitySystemLibrary::CalcArmorClass(float Dexterity, float BaseAC)
{
	// AC = 기본 AC + Dex Modifier
	const int32 DexModifier = CalcAbilityModifier(Dexterity);
	return BaseAC + static_cast<float>(DexModifier);
}

float UPBAbilitySystemLibrary::CalcInitiativeBonus(float Dexterity)
{
	// Initiative = Dex Modifier
	return static_cast<float>(CalcAbilityModifier(Dexterity));
}

TSubclassOf<UGameplayEffect> UPBAbilitySystemLibrary::GetDamageGEClass(const UObject* WorldContextObject)
{
	const UPBAbilitySystemRegistry* Registry = UPBGameInstance::GetAbilitySystemRegistry(WorldContextObject);
	if (!IsValid(Registry))
	{
		return nullptr;
	}
	return Registry->GetDamageGEClass();
}

FGameplayEffectSpecHandle UPBAbilitySystemLibrary::MakeDamageEffectSpec(
	UAbilitySystemComponent* SourceASC,
	UAbilitySystemComponent* TargetASC,
	const FPBDiceSpec& DiceSpec)
{
	if (!IsValid(SourceASC))
	{
		return FGameplayEffectSpecHandle();
	}

	const TSubclassOf<UGameplayEffect> DamageGEClass = GetDamageGEClass(SourceASC);
	if (!DamageGEClass)
	{
		return FGameplayEffectSpecHandle();
	}

	FPBDamageRollResult DamageRoll;

	switch (DiceSpec.RollType)
	{
	case EPBDiceRollType::HitRoll:
		{
			// 명중 수정치 + 숙련 보너스
			const int32 HitBonus = GetHitBonus(SourceASC, DiceSpec.KeyAttributeOverride) + GetProficiencyBonus(SourceASC);

			// TargetASC에서 ArmorClass 조회
			bool bFound = false;
			const float TargetAC = IsValid(TargetASC)
				? TargetASC->GetGameplayAttributeValue(UPBCharacterAttributeSet::GetArmorClassAttribute(), bFound)
				: 0.f;

			const FPBHitRollResult HitResult = RollHit(HitBonus, static_cast<int32>(TargetAC));
			if (!HitResult.bHit)
			{
				return FGameplayEffectSpecHandle();
			}

			const int32 AttackModifier = GetAttackModifier(SourceASC, DiceSpec.KeyAttributeOverride);
			DamageRoll = RollDamage(DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, HitResult.bCritical);
			break;
		}

	case EPBDiceRollType::SavingThrow:
		{
			// D&D 5e: 주문 데미지는 능력치 수정치 미적용
			DamageRoll.AttackModifier = 0.f;
			DamageRoll.DiceRoll = RollDamage(DiceSpec.DiceCount, DiceSpec.DiceFaces, 0.f, false).DiceRoll;

			// 시전자의 SpellSaveDC: KeyAttributeOverride 지정이면 직접 계산, 아니면 어트리뷰트 폴백
			const int32 SpellSaveDC = CalcSpellSaveDC(SourceASC, DiceSpec.KeyAttributeOverride);

			// 피주문자 내성 보너스: SaveAttributeOverride로 어떤 능력치를 쓸지 지정
			const int32 SaveBonus = GetSaveBonus(TargetASC, DiceSpec.SaveAttribute);

			const FPBSavingThrowResult SaveResult = RollSavingThrow(SaveBonus, SpellSaveDC);
			if (SaveResult.bSucceeded)
			{
				DamageRoll.DiceRoll *= 0.5f;
			}
			break;
		}

	case EPBDiceRollType::None:
	default:
		{
			const int32 AttackModifier = GetAttackModifier(SourceASC, DiceSpec.KeyAttributeOverride);
			DamageRoll = RollDamage(DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, false);
			break;
		}
	}

	const FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGEClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_DiceRoll,      DamageRoll.DiceRoll);
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_AttackModifier, DamageRoll.AttackModifier);

	return SpecHandle;
}

float UPBAbilitySystemLibrary::GetAttributeValue(AActor* Actor, FGameplayAttribute Attribute)
{
	if (!IsValid(Actor))
	{
		return 0.f;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (!IsValid(ASC))
	{
		return 0.f;
	}

	bool bFound = false;
	const float Value = ASC->GetGameplayAttributeValue(Attribute, bFound);
	return bFound ? Value : 0.f;
}

int32 UPBAbilitySystemLibrary::GetAbilityModifierValue(const UAbilitySystemComponent* ASC, FGameplayAttribute Attribute, const FGameplayAttribute& KeyAttributeOverride)
{
	if (!IsValid(ASC))
	{
		return 0;
	}

	// Override 유효: 해당 어트리뷰트의 수정치 반환
	if (KeyAttributeOverride.IsValid())
	{
		bool bFound = false;
		const float AttributeValue = ASC->GetGameplayAttributeValue(KeyAttributeOverride, bFound);
		return bFound ? CalcAbilityModifier(AttributeValue) : 0;
	}
	
	bool bFound = false;
	const float Value = ASC->GetGameplayAttributeValue(Attribute, bFound);
	return bFound ? Value : 0.f;
}

int32 UPBAbilitySystemLibrary::GetSaveBonus(const UAbilitySystemComponent* TargetASC, const FGameplayAttribute& SaveAttribute)
{
	if (!IsValid(TargetASC) || !SaveAttribute.IsValid())
	{
		return 0;
	}

	bool bFound = false;
	const float AttrValue = TargetASC->GetGameplayAttributeValue(SaveAttribute, bFound);
	return bFound ? CalcAbilityModifier(AttrValue) : 0;
}

void UPBAbilitySystemLibrary::ApplyStatsInitialization(UAbilitySystemComponent* ASC,
                                                       FPBAbilityGrantedHandles& OutHandles,
                                                       const FGameplayTag& CharacterTag, int32 CharacterLevel)
{
	const UPBAbilitySystemRegistry* AbilityRegistry = UPBGameInstance::GetAbilitySystemRegistry(ASC);
	if (!IsValid(AbilityRegistry))
	{
		return;
	}

	AbilityRegistry->ApplyStatsInitialization(ASC, OutHandles, CharacterTag, CharacterLevel);
}

void UPBAbilitySystemLibrary::ApplyCommonAbilitySet(UPBAbilitySystemComponent* ASC, int32 CharacterLevel)
{
	if (!IsValid(ASC))
	{
		return;
	}

	const UPBAbilitySystemRegistry* AbilityRegistry = UPBGameInstance::GetAbilitySystemRegistry(ASC);
	if (!IsValid(AbilityRegistry))
	{
		return;
	}

	const UPBAbilitySetData* AbilitySet = AbilityRegistry->GetCommonAbilitySet();
	if (!IsValid(AbilitySet))
	{
		return;
	}

	ASC->GrantAbilitiesFromData(PBGameplayTags::Ability_Source_Common, AbilitySet, CharacterLevel);
}

void UPBAbilitySystemLibrary::ApplyClassAbilitySet(UPBAbilitySystemComponent* ASC, const FGameplayTag& ClassTag,
                                                      int32 CharacterLevel)
{
	if (!IsValid(ASC))
	{
		return;
	}

	const UPBAbilitySystemRegistry* AbilityRegistry = UPBGameInstance::GetAbilitySystemRegistry(ASC);
	if (!IsValid(AbilityRegistry))
	{
		return;
	}

	const UPBAbilitySetData* AbilitySet = AbilityRegistry->FindAbilitySetByTag(ClassTag);
	if (!IsValid(AbilitySet))
	{
		return;
	}

	ASC->GrantAbilitiesFromData(PBGameplayTags::Ability_Source_Class, AbilitySet, CharacterLevel);
}
