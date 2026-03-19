// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBEC_Heal.h"

#include "AbilitySystemComponent.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"

UPBEC_Heal::UPBEC_Heal()
{
}

void UPBEC_Heal::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const float HealRoll = Spec.GetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Heal_Amount, false, 0.0f);

	FGameplayTagContainer SourceTags;
	if (const FGameplayTagContainer* Captured = Spec.CapturedSourceTags.GetAggregatedTags())
	{
		SourceTags = *Captured;
	}

	FGameplayTagContainer TargetTags;
	if (const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent())
	{
		TargetASC ->GetOwnedGameplayTags(TargetTags);
	}

	const float FinalHeal = UPBAbilitySystemLibrary::CalcFinalHeal(HealRoll, SourceTags, TargetTags);

	if (FinalHeal > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UPBCharacterAttributeSet::GetIncomingHealAttribute(),
		EGameplayModOp::Additive,
		FinalHeal
		));
	}

}
