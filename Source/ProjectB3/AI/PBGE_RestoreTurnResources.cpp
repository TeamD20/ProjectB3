// PBGE_RestoreTurnResources.cpp

#include "PBGE_RestoreTurnResources.h"
#include "GameplayEffectTypes.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"

UPBGE_RestoreTurnResources::UPBGE_RestoreTurnResources()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Action 복원: 1.0f 고정값
	FGameplayModifierInfo RestoreActionInfo;
	RestoreActionInfo.Attribute =
		UPBTurnResourceAttributeSet::GetActionAttribute();
	RestoreActionInfo.ModifierOp = EGameplayModOp::Override;
	FScalableFloat ActionValue;
	ActionValue.SetValue(1.0f);
	RestoreActionInfo.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(ActionValue);
	Modifiers.Add(RestoreActionInfo);

	// BonusAction 복원: 1.0f 고정값
	FGameplayModifierInfo RestoreBonusActionInfo;
	RestoreBonusActionInfo.Attribute =
		UPBTurnResourceAttributeSet::GetBonusActionAttribute();
	RestoreBonusActionInfo.ModifierOp = EGameplayModOp::Override;
	FScalableFloat BonusActionValue;
	BonusActionValue.SetValue(1.0f);
	RestoreBonusActionInfo.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(BonusActionValue);
	Modifiers.Add(RestoreBonusActionInfo);

	// Reaction 복원: 1.0f 고정값
	FGameplayModifierInfo RestoreReactionInfo;
	RestoreReactionInfo.Attribute =
		UPBTurnResourceAttributeSet::GetReactionAttribute();
	RestoreReactionInfo.ModifierOp = EGameplayModOp::Override;
	FScalableFloat ReactionValue;
	ReactionValue.SetValue(1.0f);
	RestoreReactionInfo.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(ReactionValue);
	Modifiers.Add(RestoreReactionInfo);

	// Movement 복원: 900.0f 고정값 (9m)
	FGameplayModifierInfo RestoreMovementInfo;
	RestoreMovementInfo.Attribute =
		UPBTurnResourceAttributeSet::GetMovementAttribute();
	RestoreMovementInfo.ModifierOp = EGameplayModOp::Override;
	FScalableFloat MovementValue;
	MovementValue.SetValue(900.0f);
	RestoreMovementInfo.ModifierMagnitude =
		FGameplayEffectModifierMagnitude(MovementValue);
	Modifiers.Add(RestoreMovementInfo);
}
