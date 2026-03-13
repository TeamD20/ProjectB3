// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "PBAbilityTask_FaceTarget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPBFaceTargetCompleted);

/**
 * 지정한 위치 또는 액터를 향해 Duration 동안 부드럽게 Yaw 회전하는 AbilityTask.
 * Duration 경과 또는 목표 방향과의 각도 오차가 임계값 이내 진입 시 OnCompleted를 브로드캐스트한다.
 */
UCLASS()
class PROJECTB3_API UPBAbilityTask_FaceTarget : public UAbilityTask
{
	GENERATED_BODY()

public:
	// 목표 위치를 향해 Duration 동안 부드럽게 Yaw 회전 (Duration <= 0이면 각도 임계값 도달 시 즉시 완료)
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UPBAbilityTask_FaceTarget* FaceTargetLocation(
		UGameplayAbility* OwningAbility,
		FVector InTargetLocation,
		float InDuration = 0.3f,
		float InInterpSpeed = 10.0f);

	// 목표 액터를 향해 Duration 동안 부드럽게 Yaw 회전 (매 틱 액터 위치를 추적)
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UPBAbilityTask_FaceTarget* FaceTargetActor(
		UGameplayAbility* OwningAbility,
		AActor* InTargetActor,
		float InDuration = 0.3f,
		float InInterpSpeed = 10.0f);

	// 회전 완료 델리게이트 (Duration 경과 또는 각도 임계값 도달 시 발생)
	UPROPERTY(BlueprintAssignable)
	FOnPBFaceTargetCompleted OnCompleted;

protected:
	/*~ UAbilityTask Interface ~*/
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	// 이번 틱의 목표 Yaw 계산 (액터 추적 여부에 따라 TargetLocation 또는 WeakTargetActor 위치 사용)
	bool TryGetTargetYaw(float& OutYaw) const;

	// 목표 위치 (FaceTargetLocation 사용 시)
	FVector TargetLocation = FVector::ZeroVector;

	// 목표 액터 (FaceTargetActor 사용 시, 약참조)
	TWeakObjectPtr<AActor> WeakTargetActor;

	// 최대 회전 시간 (초). 0 이하면 각도 임계값 도달 시 즉시 완료
	float Duration = 0.3f;

	// 보간 속도 (RInterpTo에 전달)
	float InterpSpeed = 10.0f;

	// 경과 시간
	float ElapsedTime = 0.0f;

	// 액터 추적 여부
	bool bUseTargetActor = false;
};
