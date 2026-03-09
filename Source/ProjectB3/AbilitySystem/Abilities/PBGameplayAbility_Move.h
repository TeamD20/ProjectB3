// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBGameplayAbility.h"
#include "PBGameplayAbility_Move.generated.h"

/**
 * 클릭 이동 어빌리티.
 * 발동 시 PC를 Movement 모드로 전환하고,
 * Event.Movement.MoveCommand 이벤트를 수신할 때마다 목적지로 이동한다.
 */
UCLASS()
class PROJECTB3_API UPBGameplayAbility_Move : public UPBGameplayAbility
{
	GENERATED_BODY()

public:
	UPBGameplayAbility_Move();

protected:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	// MoveCommand 이벤트 수신 핸들러
	UFUNCTION()
	void HandleMoveEvent(FGameplayEventData Payload);

	// NavPath 포인트에서 MaxMoveDistance 기준 최종 목적지 계산
	FVector CalculateClampedDestination(const TArray<FVector>& PathPoints, float MaxDist) const;
};
