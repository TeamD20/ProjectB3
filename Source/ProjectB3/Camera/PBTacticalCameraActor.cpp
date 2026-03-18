// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTacticalCameraActor.h"
#include "Camera/CameraComponent.h"

APBTacticalCameraActor::APBTacticalCameraActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TacticalCamera"));
	RootComponent = CameraComponent;
}

void APBTacticalCameraActor::SetTrackingTarget(AActor* InTarget, float InYaw, float InPitch, float InDistance)
{
	TrackingTarget = InTarget;
	TrackYaw       = InYaw;
	TrackPitch     = InPitch;
	TrackDistance  = InDistance;
	SetActorTickEnabled(true);
}

void APBTacticalCameraActor::SetTargetTransform(const FTransform& InTargetTransform)
{
	// 고정 목표로 이동 — 추적 해제
	TrackingTarget.Reset();
	TargetTransform = InTargetTransform;
	SetActorTickEnabled(true);
}

void APBTacticalCameraActor::ClearTracking()
{
	TrackingTarget.Reset();
	// 현재 위치를 TargetTransform으로 고정하여 제자리 유지
	TargetTransform = FTransform(GetActorRotation(), GetActorLocation());
}

void APBTacticalCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Tracking 모드: 매 틱 대상 위치 기준으로 Orbit TargetTransform 재계산
	if (TrackingTarget.IsValid())
	{
		const FVector FocusPoint = TrackingTarget->GetActorLocation();
		const FVector NewCamLoc  = FocusPoint + FRotator(TrackPitch, TrackYaw, 0.0f).Vector() * -TrackDistance;

		// 현재 회전에서 최단경로 회전 계산
		const FRotator RawRot    = (FocusPoint - NewCamLoc).Rotation();
		const FRotator Delta     = (RawRot - GetActorRotation()).GetNormalized();
		const FRotator NewCamRot = GetActorRotation() + Delta;

		TargetTransform = FTransform(NewCamRot, NewCamLoc);
	}

	const FVector NewLocation = FMath::VInterpTo(
		GetActorLocation(), TargetTransform.GetLocation(), DeltaTime, InterpSpeed);
	const FRotator NewRotation = FMath::RInterpTo(
		GetActorRotation(), TargetTransform.GetRotation().Rotator(), DeltaTime, InterpSpeed);

	SetActorLocationAndRotation(NewLocation, NewRotation);
}
