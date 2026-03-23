// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEC_Damage.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/PBGameplayTags.h"

UPBEC_Damage::UPBEC_Damage()
{
	// 현재는 Target 어트리뷰트 캡처 없음.
	// 저항/취약은 태그 기반으로 처리하므로 별도 캡처 불필요.
}

void UPBEC_Damage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 어빌리티에서 전달된 주사위 결과 (치명타 적용 완료된 값)
	const float DiceRoll = Spec.GetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_DiceRoll, false, 0.f);

	// 어빌리티에서 전달된 공격 수정치 (Str 또는 Dex modifier)
	const float AttackModifier = Spec.GetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_AttackModifier, false, 0.f);

	// Source 태그 (치명타 여부 등 어빌리티 컨텍스트)
	FGameplayTagContainer SourceTags;
	if (const FGameplayTagContainer* Captured = Spec.CapturedSourceTags.GetAggregatedTags())
	{
		SourceTags = *Captured;
	}

	// Target 태그 (저항·취약 등 대상 상태)
	FGameplayTagContainer TargetTags;
	if (const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent())
	{
		TargetASC->GetOwnedGameplayTags(TargetTags);
	}

	const float FinalDamage = UPBAbilitySystemLibrary::CalcFinalDamage(DiceRoll, AttackModifier, SourceTags, TargetTags);

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UPBCharacterAttributeSet::GetIncomingDamageAttribute(),
			EGameplayModOp::Additive,
			FinalDamage
		));
}
