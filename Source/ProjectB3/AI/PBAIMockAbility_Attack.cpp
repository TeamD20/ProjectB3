// PBAIMockAbility_Attack.cpp
#include "PBAIMockAbility_Attack.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"

UPBAIMockAbility_Attack::UPBAIMockAbility_Attack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 태그 설정
	AbilityTags.AddTag(
		FGameplayTag::RequestGameplayTag(FName("Ability.Attack.Melee")));
}

void UPBAIMockAbility_Attack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Action 자원 차감은 PBExecuteSequenceTask에서 이미 처리
	// (ApplyModToAttributeUnsafe로 ActionCost만큼 차감)
	// 여기서 중복 차감 시 Attack 1회당 -2.0f 발생하므로 제거
	UE_LOG(LogTemp, Display,
	       TEXT(">> [MOCK GAS] AI Attack! Swung weapon at target."));

	// 반드시 즉시 종료시켜야 StateTree가 뻗거나 꼬임 방지

	bool bReplicateEndAbility = true;
	bool bWasCancelled = false;
	EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility,
	           bWasCancelled);
}
