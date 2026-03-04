// PBAIMockAbility_Attack.cpp
#include "PBAIMockAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "PBAIMockAttributeSet.h"

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
    // AP 감소로직 ( 임시 )
   
    const UPBAIMockAttributeSet *MockSet = Cast<UPBAIMockAttributeSet>(
        ASC->GetAttributeSet(UPBAIMockAttributeSet::StaticClass()));
    if (MockSet) {
      float CurrentAP = MockSet->GetActionPoints();
      if (CurrentAP >= 1.0f) {
        // 단순 로깅 (공격 성공)
        UE_LOG(LogTemp, Display,
               TEXT(">> [MOCK GAS] AI Attack! Swung weapon. Remaining AP: %f"),
               CurrentAP - 1.0f);

        // ApplyModToAttribute를 통한 강제 직접 감산
        ASC->ApplyModToAttribute(
            UPBAIMockAttributeSet::GetActionPointsAttribute(),
            EGameplayModOp::Additive, -1.0f);
      } else {
        UE_LOG(LogTemp, Warning,
               TEXT(">> [MOCK GAS] Not enough AP! Need 1, have %f"), CurrentAP);
      }
    }
  }

  // 반드시 즉시 종료시켜야 StateTree가 뻗거나 꼬임 방지
  bool bReplicateEndAbility = true;
  bool bWasCancelled = false;
  EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility,
             bWasCancelled);
}
