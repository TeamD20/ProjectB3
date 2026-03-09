// PBAIMockAbility_Attack.cpp
#include "PBAIMockAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"

UPBAIMockAbility_Attack::UPBAIMockAbility_Attack() {
  InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

  // 태그 설정
  AbilityTags.AddTag(
      FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
}

void UPBAIMockAbility_Attack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo *ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData *TriggerEventData) {
  Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

  UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
  if (ASC) {
    // Action 소모 로직
    const UPBTurnResourceAttributeSet *TurnSet =
        Cast<UPBTurnResourceAttributeSet>(
            ASC->GetAttributeSet(UPBTurnResourceAttributeSet::StaticClass()));
    if (TurnSet) {
      float CurrentAction = TurnSet->GetAction();
      if (CurrentAction >= 1.0f) {
        // 단순 로깅 (공격 성공)
        UE_LOG(
            LogTemp, Display,
            TEXT(">> [MOCK GAS] AI Attack! Swung weapon. Remaining Action: %f"),
            CurrentAction - 1.0f);

        // ApplyModToAttribute를 통한 강제 직접 감산
        ASC->ApplyModToAttribute(
            UPBTurnResourceAttributeSet::GetActionAttribute(),
            EGameplayModOp::Additive, -1.0f);
      } else {
        UE_LOG(LogTemp, Warning,
               TEXT(">> [MOCK GAS] Not enough Action! Need 1, have %f"),
               CurrentAction);
      }
    }
  }

  // 반드시 즉시 종료시켜야 StateTree가 뻗거나 꼬임 방지
  bool bReplicateEndAbility = true;
  bool bWasCancelled = false;
  EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility,
             bWasCancelled);
}
