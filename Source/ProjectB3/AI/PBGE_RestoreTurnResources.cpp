// PBGE_RestoreTurnResources.cpp

#include "PBGE_RestoreTurnResources.h"
#include "GameplayEffectTypes.h"
#include "PBAIMockAttributeSet.h"

UPBGE_RestoreTurnResources::UPBGE_RestoreTurnResources() {
  DurationPolicy = EGameplayEffectDurationType::Instant;

  // Action 속성을 MaxAction 값으로 오버라이드 복원
  FGameplayModifierInfo RestoreActionInfo;
  RestoreActionInfo.Attribute = UPBAIMockAttributeSet::GetActionAttribute();
  RestoreActionInfo.ModifierOp = EGameplayModOp::Override;
  FAttributeBasedFloat ActionBasedFloat;
  ActionBasedFloat.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
      UPBAIMockAttributeSet::GetMaxActionAttribute(),
      EGameplayEffectAttributeCaptureSource::Source, false);
  ActionBasedFloat.AttributeCalculationType =
      EAttributeBasedFloatCalculationType::AttributeMagnitude;

  RestoreActionInfo.ModifierMagnitude =
      FGameplayEffectModifierMagnitude(ActionBasedFloat);
  Modifiers.Add(RestoreActionInfo);

  // BonusAction 속성을 MaxBonusAction 값으로 오버라이드 복원
  FGameplayModifierInfo RestoreBonusActionInfo;
  RestoreBonusActionInfo.Attribute =
      UPBAIMockAttributeSet::GetBonusActionAttribute();
  RestoreBonusActionInfo.ModifierOp = EGameplayModOp::Override;
  FAttributeBasedFloat BonusActionBasedFloat;
  BonusActionBasedFloat.BackingAttribute =
      FGameplayEffectAttributeCaptureDefinition(
          UPBAIMockAttributeSet::GetMaxBonusActionAttribute(),
          EGameplayEffectAttributeCaptureSource::Source, false);
  BonusActionBasedFloat.AttributeCalculationType =
      EAttributeBasedFloatCalculationType::AttributeMagnitude;

  RestoreBonusActionInfo.ModifierMagnitude =
      FGameplayEffectModifierMagnitude(BonusActionBasedFloat);
  Modifiers.Add(RestoreBonusActionInfo);

  // Movement 속성을 MaxMovement 값으로 오버라이드 복원
  FGameplayModifierInfo RestoreMovementInfo;
  RestoreMovementInfo.Attribute = UPBAIMockAttributeSet::GetMovementAttribute();
  RestoreMovementInfo.ModifierOp = EGameplayModOp::Override;
  FAttributeBasedFloat MovementBasedFloat;
  MovementBasedFloat.BackingAttribute =
      FGameplayEffectAttributeCaptureDefinition(
          UPBAIMockAttributeSet::GetMaxMovementAttribute(),
          EGameplayEffectAttributeCaptureSource::Source, false);
  MovementBasedFloat.AttributeCalculationType =
      EAttributeBasedFloatCalculationType::AttributeMagnitude;

  RestoreMovementInfo.ModifierMagnitude =
      FGameplayEffectModifierMagnitude(MovementBasedFloat);
  Modifiers.Add(RestoreMovementInfo);
}
