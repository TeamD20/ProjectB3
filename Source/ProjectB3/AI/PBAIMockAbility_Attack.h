// PBAIMockAbility_Attack.h
#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"
#include "PBAIMockAbility_Attack.generated.h"


UCLASS()
class PROJECTB3_API UPBAIMockAbility_Attack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPBAIMockAbility_Attack();

protected:
	virtual void
	ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                const FGameplayAbilityActorInfo* ActorInfo,
	                const FGameplayAbilityActivationInfo ActivationInfo,
	                const FGameplayEventData* TriggerEventData) override;
};
