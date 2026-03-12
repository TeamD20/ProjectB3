// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBGameplayAbility, Log, All);

EPBAbilityType UPBGameplayAbility::GetAbilityType() const
{
	// CDO가 아닌 인스턴스에서만 Spec의 DynamicAbilityTags를 포함하여 조회한다.
	if (IsInstantiated())
	{
		if (FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
		{
			FGameplayTagContainer CombinedTags;
			CombinedTags.AppendTags(GetAssetTags());
			CombinedTags.AppendTags(Spec->GetDynamicSpecSourceTags());
			return GetAbilityTypeFromTags(CombinedTags);
		}
	}

	// CDO이거나 활성 Spec이 없으면 AssetTags만 조회한다.
	return GetAbilityTypeFromTags(GetAssetTags());
}

bool UPBGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 타입이 None이면 제한 없이 활성화 허용
	if (GetAbilityType() == EPBAbilityType::None)
	{
		return true;
	}

	// 턴 기반 어빌리티가 이미 실행 중이면 활성화 거부
	if (const UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(
		ActorInfo->AbilitySystemComponent.Get()))
	{
		if (PBASC->IsTurnAbilityActive())
		{
			return false;
		}
	}

	return true;
}

void UPBGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 턴 기반 어빌리티인 경우 ASC에 실행 중 플래그 설정
	if (GetAbilityType() != EPBAbilityType::None)
	{
		if (UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()))
		{
			PBASC->SetTurnAbilityActive(true);
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPBGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 턴 기반 어빌리티인 경우 ASC 플래그 해제
	if (GetAbilityType() != EPBAbilityType::None)
	{
		if (UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()))
		{
			PBASC->SetTurnAbilityActive(false);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UGameplayEffect* UPBGameplayAbility::GetCooldownGameplayEffect() const
{
	// TODO: 턴제 쿨다운은 GAS 내장 CooldownTag GE를 사용하지 않는다.
	// 추후 턴 종료 시점에 별도 쿨다운 카운터로 관리 예정.
	return nullptr;
}

FPBHitRollResult UPBGameplayAbility::RollHit(const UAbilitySystemComponent* InTargetASC) const
{
	if (DiceSpec.RollType != EPBDiceRollType::HitRoll)
	{
		UE_LOG(LogPBGameplayAbility, Warning, TEXT("DiceSpec의 RollType이 HitRoll이 아니지만 RollHit()을 호출함"));
	}
	
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	// 명중 수정치(숙련 제외) + 숙련 보너스
	const int32 HitBonus = UPBAbilitySystemLibrary::GetHitBonus(SourceASC, DiceSpec.BonusAttributeOverride) 
		+ UPBAbilitySystemLibrary::GetProficiencyBonus(SourceASC);

	bool bFound = false;
	const float TargetACValue = IsValid(InTargetASC)
		? InTargetASC->GetGameplayAttributeValue(UPBCharacterAttributeSet::GetArmorClassAttribute(), bFound)
		: 0.f;
	const int32 TargetAC = static_cast<int32>(TargetACValue);

	const FPBHitRollResult HitRollResult = UPBAbilitySystemLibrary::RollHit(HitBonus, TargetAC);
	UE_LOG(LogPBGameplayAbility, Warning,
		TEXT("[%s] RollHit - Roll: %d, HitBonus: %d, TargetAC: %d, bHit: %s, bCritical: %s"),
		*GetName(),
		HitRollResult.Roll,
		HitBonus,
		TargetAC,
		HitRollResult.bHit ? TEXT("true") : TEXT("false"),
		HitRollResult.bCritical ? TEXT("true") : TEXT("false"));

	// TODO: UI 시스템 연동
	return HitRollResult;
}

FPBDamageRollResult UPBGameplayAbility::RollDamage(bool bCritical) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAttribute AttackModifierAttributeOverride = DiceSpec.AttackModifierAttributeOverride;
	// AttackModifier = KeyAttribute 수정치 (숙련 보너스는 데미지에 미포함)
	const float AttackModifier =  UPBAbilitySystemLibrary::GetAttackModifier(SourceASC, AttackModifierAttributeOverride);
	const FPBDamageRollResult DamageRollResult = UPBAbilitySystemLibrary::RollDamage(DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, bCritical);
	UE_LOG(LogPBGameplayAbility, Warning,
		TEXT("[%s] RollDamage - Dice: %dd%d, bCritical: %s, DiceRoll: %.2f, AttackModifier: %.2f"),
		*GetName(),
		DiceSpec.DiceCount,
		DiceSpec.DiceFaces,
		bCritical ? TEXT("true") : TEXT("false"),
		DamageRollResult.DiceRoll,
		DamageRollResult.AttackModifier);

	// TODO: UI 시스템 연동
	return DamageRollResult;
}

FPBSavingThrowResult UPBGameplayAbility::RollSavingThrow(const UAbilitySystemComponent* InTargetASC) const
{
	if (DiceSpec.RollType != EPBDiceRollType::SavingThrow)
	{
		UE_LOG(LogPBGameplayAbility, Warning, TEXT("DiceSpec의 RollType이 SavingThrow 아니지만 RollSavingThrow()을 호출함"));
	}
	
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	// SaveBonus = SaveAttributeOverride 능력치 수정치 (숙련 제외)
	const int32 SaveBonus = UPBAbilitySystemLibrary::GetSaveBonus(InTargetASC, DiceSpec.TargetSaveAttribute);
	const int32 SpellSaveDC = UPBAbilitySystemLibrary::CalcSpellSaveDC(SourceASC, DiceSpec.BonusAttributeOverride);
	const FPBSavingThrowResult SavingThrowResult = UPBAbilitySystemLibrary::RollSavingThrow(SaveBonus, SpellSaveDC);
	UE_LOG(LogPBGameplayAbility, Warning,
		TEXT("[%s] RollSavingThrow - Roll: %d, SaveBonus: %d, SpellSaveDC: %d, bSucceeded: %s"),
		*GetName(),
		SavingThrowResult.Roll,
		SaveBonus,
		SpellSaveDC,
		SavingThrowResult.bSucceeded ? TEXT("true") : TEXT("false"));

	// TODO: UI 시스템 연동
	return SavingThrowResult;
}

FGameplayEffectSpecHandle UPBGameplayAbility::MakeDamageEffectSpecFromHitDamageRoll(
	const UAbilitySystemComponent* InTargetASC, FPBHitRollResult& OutHitRollResult,
	FPBDamageRollResult& OutDamageRollResult) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC) || !IsValid(InTargetASC))
	{
		return FGameplayEffectSpecHandle();
	}

	const TSubclassOf<UGameplayEffect> DamageGEClass = UPBAbilitySystemLibrary::GetDamageGEClass(SourceASC);
	if (!DamageGEClass)
	{
		return FGameplayEffectSpecHandle();
	}

	const FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGEClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}
	
	// 명중 굴림
	OutHitRollResult = RollHit(InTargetASC);
	if (!OutHitRollResult.bHit)
	{
		return FGameplayEffectSpecHandle();
	}
	
	// 데미지 굴림
	OutDamageRollResult = RollDamage(OutHitRollResult.bCritical);
	
	// 스펙에 반영
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_DiceRoll,      OutDamageRollResult.DiceRoll);
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_AttackModifier, OutDamageRollResult.AttackModifier);
	
	return SpecHandle;
}

FGameplayEffectSpecHandle UPBGameplayAbility::MakeDamageEffectSpecFromSavingThrowDamageRoll(
	const UAbilitySystemComponent* InTargetASC, FPBSavingThrowResult& OutSavingThrowResult,
	FPBDamageRollResult& OutDamageRollResult) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC) || !IsValid(InTargetASC))
	{
		return FGameplayEffectSpecHandle();
	}

	const TSubclassOf<UGameplayEffect> DamageGEClass = UPBAbilitySystemLibrary::GetDamageGEClass(SourceASC);
	if (!DamageGEClass)
	{
		return FGameplayEffectSpecHandle();
	}
	
	const FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGEClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}
	
	// 데미지 굴림
	OutDamageRollResult = RollDamage(false);
	// 내성 굴림
	OutSavingThrowResult = RollSavingThrow(InTargetASC);
	if (OutSavingThrowResult.bSucceeded)
	{
		OutDamageRollResult.DiceRoll *= 0.5f;
	}
	
	// 스펙에 반영
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_DiceRoll,      OutDamageRollResult.DiceRoll);
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_AttackModifier, OutDamageRollResult.AttackModifier);
	
	return SpecHandle;
}

float UPBGameplayAbility::GetExpectedHitDamage(const UAbilitySystemComponent* InTargetASC,
                                               const FGameplayTagContainer& SourceTags,
                                               const FGameplayTagContainer& TargetTags) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	// AttackModifier: 데미지 수정치 (숙련 미포함)
	const float AttackModifier = static_cast<float>(UPBAbilitySystemLibrary::GetAttackModifier(SourceASC,  DiceSpec.AttackModifierAttributeOverride));
	// HitBonus: 명중 수정치 + 숙련 보너스
	const int32 HitBonus = UPBAbilitySystemLibrary::GetHitBonus(SourceASC, DiceSpec.BonusAttributeOverride)
		+ UPBAbilitySystemLibrary::GetProficiencyBonus(SourceASC);
	
	bool bFound = false;
	const float TargetACValue = IsValid(InTargetASC)
		? InTargetASC->GetGameplayAttributeValue(UPBCharacterAttributeSet::GetArmorClassAttribute(), bFound)
		: 0.f;
	const int32 TargetAC = static_cast<int32>(TargetACValue);

	return UPBAbilitySystemLibrary::CalcExpectedAttackDamage(
		DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, HitBonus, TargetAC, SourceTags, TargetTags);
}

float UPBGameplayAbility::GetExpectedSavingThrowDamage(const UAbilitySystemComponent* InTargetASC,
                                                       const FGameplayTagContainer& SourceTags,
                                                       const FGameplayTagContainer& TargetTags) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const float AttackModifier = static_cast<float>(UPBAbilitySystemLibrary::GetAttackModifier(SourceASC, DiceSpec.AttackModifierAttributeOverride));
	const int32 SaveBonus = UPBAbilitySystemLibrary::GetSaveBonus(InTargetASC, DiceSpec.TargetSaveAttribute);
	const int32 SpellSaveDC = UPBAbilitySystemLibrary::CalcSpellSaveDC(SourceASC, DiceSpec.BonusAttributeOverride);
	
	return UPBAbilitySystemLibrary::CalcExpectedSavingThrowDamage(
		DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, SaveBonus, SpellSaveDC, SourceTags, TargetTags);
}

float UPBGameplayAbility::GetExpectedDirectDamage(
	const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& TargetTags) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float AttackModifier = static_cast<float>(UPBAbilitySystemLibrary::GetAttackModifier(ASC,  DiceSpec.AttackModifierAttributeOverride));
	
	return UPBAbilitySystemLibrary::CalcExpectedDamage(
		DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, SourceTags, TargetTags);
}

FPBAbilityTargetData UPBGameplayAbility::ExtractTargetDataFromEvent(
	const FGameplayEventData& EventData) const
{
	if (const UPBTargetPayload* Payload = Cast<UPBTargetPayload>(EventData.OptionalObject))
	{
		return Payload->TargetData;
	}

	return FPBAbilityTargetData();
}
