// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBGameplayAbility.h"
#include "Navigation/PathFollowingComponent.h"
class UPBAbilityTask_MoveToLocation;
class UPBEnvironmentSubsystem;

#include "PBGameplayAbility_Move.generated.h"

/**
 * 이동 어빌리티.
 * Event.Movement.MoveCommand 이벤트를 수신하면 목적지로 이동한다.
 * 플레이어: 발동 시 PC를 Movement 모드로 전환
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

	// 이동 완료 핸들러
	UFUNCTION()
	void HandleMoveCompleted(TEnumAsByte<EPathFollowingResult::Type> Result);

	// EnvironmentSubsystem 캐싱 참조 취득
	UPBEnvironmentSubsystem* GetEnvironmentSubsystem() const;

	// 현재 실행 중인 이동 Task
	UPROPERTY()
	TObjectPtr<UPBAbilityTask_MoveToLocation> ActiveMoveTask;

	// 이동 경로 포인트 (실제 이동 거리 계산용)
	TArray<FVector> MovePathPoints;
};
