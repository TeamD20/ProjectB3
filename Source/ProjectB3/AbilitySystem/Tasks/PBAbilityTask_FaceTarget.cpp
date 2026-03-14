// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilityTask_FaceTarget.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

// 회전 완료로 간주하는 목표 방향과의 최대 각도 오차 (도)
static constexpr float FaceTargetAngleThreshold = 2.0f;

UPBAbilityTask_FaceTarget* UPBAbilityTask_FaceTarget::FaceTargetLocation(
	UGameplayAbility* OwningAbility, FVector InTargetLocation, float InDuration, float InInterpSpeed)
{
	UPBAbilityTask_FaceTarget* Task = NewAbilityTask<UPBAbilityTask_FaceTarget>(OwningAbility);
	Task->TargetLocation = InTargetLocation;
	Task->Duration       = InDuration;
	Task->InterpSpeed    = InInterpSpeed;
	Task->bUseTargetActor = false;
	return Task;
}

UPBAbilityTask_FaceTarget* UPBAbilityTask_FaceTarget::FaceTargetActor(
	UGameplayAbility* OwningAbility, AActor* InTargetActor, float InDuration, float InInterpSpeed)
{
	UPBAbilityTask_FaceTarget* Task = NewAbilityTask<UPBAbilityTask_FaceTarget>(OwningAbility);
	Task->WeakTargetActor = InTargetActor;
	Task->Duration        = InDuration;
	Task->InterpSpeed     = InInterpSpeed;
	Task->bUseTargetActor = true;
	return Task;
}

void UPBAbilityTask_FaceTarget::Activate()
{
	AActor* Avatar = GetAvatarActor();
	if (!IsValid(Avatar))
	{
		OnCompleted.Broadcast();
		EndTask();
		return;
	}

	bTickingTask = true;
	ElapsedTime  = 0.0f;
}

void UPBAbilityTask_FaceTarget::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	AActor* Avatar = GetAvatarActor();
	if (!IsValid(Avatar))
	{
		OnCompleted.Broadcast();
		EndTask();
		return;
	}

	float TargetYaw = 0.0f;
	if (!TryGetTargetYaw(TargetYaw))
	{
		// 타겟 액터가 소멸된 경우 현재 상태로 완료
		OnCompleted.Broadcast();
		EndTask();
		return;
	}

	// 현재 Yaw에서 목표 Yaw로 보간
	const FRotator CurrentRotation = Avatar->GetActorRotation();
	const FRotator TargetRotation  = FRotator(0.0f, TargetYaw, 0.0f);
	const FRotator NewRotation     = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, InterpSpeed);

	Avatar->SetActorRotation(NewRotation);

	ElapsedTime += DeltaTime;

	// 각도 오차가 임계값 이내에 들어오면 완료
	const float AngleDiff = FMath::Abs(FRotator::NormalizeAxis(TargetYaw - NewRotation.Yaw));
	if (AngleDiff <= FaceTargetAngleThreshold)
	{
		// 목표 방향으로 정확히 스냅
		Avatar->SetActorRotation(TargetRotation);
		OnCompleted.Broadcast();
		EndTask();
		return;
	}

	// Duration이 0보다 크면 경과 시간 기준 완료
	if (Duration > 0.0f && ElapsedTime >= Duration)
	{
		OnCompleted.Broadcast();
		EndTask();
	}
}

void UPBAbilityTask_FaceTarget::OnDestroy(bool bInOwnerFinished)
{
	Super::OnDestroy(bInOwnerFinished);
}

bool UPBAbilityTask_FaceTarget::TryGetTargetYaw(float& OutYaw) const
{
	AActor* Avatar = GetAvatarActor();
	if (!IsValid(Avatar))
	{
		return false;
	}

	FVector ToTarget;

	if (bUseTargetActor)
	{
		const AActor* TargetActor = WeakTargetActor.Get();
		if (!IsValid(TargetActor))
		{
			return false;
		}
		ToTarget = TargetActor->GetActorLocation() - Avatar->GetActorLocation();
	}
	else
	{
		ToTarget = TargetLocation - Avatar->GetActorLocation();
	}

	// 수평 방향만 고려 (Z 무시)
	ToTarget.Z = 0.0f;
	if (ToTarget.IsNearlyZero())
	{
		// 이미 같은 위치: 현재 Yaw 유지
		OutYaw = Avatar->GetActorRotation().Yaw;
		return true;
	}

	OutYaw = ToTarget.Rotation().Yaw;
	return true;
}
