// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"

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

FPBHitRollResult UPBGameplayAbility::RollHit(const UAbilitySystemComponent* InSourceASC, const UAbilitySystemComponent* InTargetASC) const
{
	// 명중 수정치(숙련 제외) + 숙련 보너스
	const int32 HitBonus = UPBAbilitySystemLibrary::GetHitBonus(InSourceASC, DiceSpec.KeyAttributeOverride) 
		+ UPBAbilitySystemLibrary::GetProficiencyBonus(InSourceASC);

	bool bFound = false;
	const float TargetACValue = IsValid(InTargetASC)
		? InTargetASC->GetGameplayAttributeValue(UPBCharacterAttributeSet::GetArmorClassAttribute(), bFound)
		: 0.f;
	const int32 TargetAC = bFound ? static_cast<int32>(TargetACValue) : 0;

	// TODO: UI 시스템 연동
	return UPBAbilitySystemLibrary::RollHit(HitBonus, TargetAC);
}

FPBDamageRollResult UPBGameplayAbility::RollDamage(const UAbilitySystemComponent* InSourceASC, bool bCritical) const
{
	// AttackModifier = KeyAttribute 수정치 (숙련 보너스는 데미지에 미포함)
	const float AttackModifier =  UPBAbilitySystemLibrary::GetAttackModifier(InSourceASC, DiceSpec.KeyAttributeOverride);
	// TODO: UI 시스템 연동
	return UPBAbilitySystemLibrary::RollDamage(DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, bCritical);
}

FPBSavingThrowResult UPBGameplayAbility::RollSavingThrow(const UAbilitySystemComponent* InSourceASC, const UAbilitySystemComponent* InTargetASC) const
{
	// SaveBonus = SaveAttributeOverride 능력치 수정치 (숙련 제외)
	const int32 SaveBonus = UPBAbilitySystemLibrary::GetSaveBonus(InTargetASC, DiceSpec.SaveAttribute);
	const int32 DC = UPBAbilitySystemLibrary::CalcSpellSaveDC(InSourceASC, DiceSpec.KeyAttributeOverride);
	// TODO: UI 시스템 연동
	
	return UPBAbilitySystemLibrary::RollSavingThrow(SaveBonus, DC);
}

float UPBGameplayAbility::GetExpectedHitDamage(int32 TargetAC,
                                               const FGameplayTagContainer& SourceTags,
                                               const FGameplayTagContainer& TargetTags) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	// AttackModifier: 데미지 수정치 (숙련 미포함)
	const float AttackModifier = static_cast<float>(UPBAbilitySystemLibrary::GetAttackModifier(ASC, DiceSpec.KeyAttributeOverride));
	// HitBonus: 명중 수정치 + 숙련 보너스
	const int32 HitBonus = UPBAbilitySystemLibrary::GetHitBonus(ASC, DiceSpec.KeyAttributeOverride)
		+ UPBAbilitySystemLibrary::GetProficiencyBonus(ASC);

	return UPBAbilitySystemLibrary::CalcExpectedAttackDamage(
		DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, HitBonus, TargetAC, SourceTags, TargetTags);
}

float UPBGameplayAbility::GetExpectedSavingThrowDamage(int32 SpellSaveDC,
                                                       const FGameplayTagContainer& SourceTags,
                                                       const FGameplayTagContainer& TargetTags) const
{
	// D&D 5e: 주문 데미지는 수정치 미적용
	constexpr float AttackModifier = 0.f;
	// AI 기댓값 계산 용도 → 피주문자 ASC 없음, SaveBonus = 0으로 최악(최대 피해) 기준 계산
	constexpr int32 SaveBonus = 0;
	
	return UPBAbilitySystemLibrary::CalcExpectedSavingThrowDamage(
		DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, SaveBonus, SpellSaveDC, SourceTags, TargetTags);
}

float UPBGameplayAbility::GetExpectedDirectDamage(
	const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& TargetTags) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float AttackModifier = static_cast<float>(UPBAbilitySystemLibrary::GetAttackModifier(ASC, DiceSpec.KeyAttributeOverride));
	
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
