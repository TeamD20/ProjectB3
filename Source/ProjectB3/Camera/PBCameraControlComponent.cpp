// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCameraControlComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

UPBCameraControlComponent::UPBCameraControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPBCameraControlComponent::SetSpringArm(USpringArmComponent* InSpringArm)
{
	CachedSpringArm = InSpringArm;
}

void UPBCameraControlComponent::SetModeParams(const FPBCameraModeParams& NewParams)
{
	CurrentModeParams = NewParams;

	// 줌 레벨을 새 범위로 클램프
	TargetZoomLevel = FMath::Clamp(TargetZoomLevel, CurrentModeParams.ZoomMin, CurrentModeParams.ZoomMax);

	// Yaw를 새 범위로 클램프
	TargetYaw = ClampYaw(TargetYaw);
}

void UPBCameraControlComponent::ResetOffset()
{
	TargetCameraOffset = FVector::ZeroVector;
	bIsResettingOffset = true;
}

void UPBCameraControlComponent::AddZoomInput(float AxisValue)
{
	TargetZoomLevel = FMath::Clamp(
		TargetZoomLevel + AxisValue * ZoomStep,
		CurrentModeParams.ZoomMin,
		CurrentModeParams.ZoomMax
	);
}

void UPBCameraControlComponent::AddRotationStepInput(float AxisValue)
{
	TargetYaw = ClampYaw(TargetYaw + AxisValue * RotationStepDegrees);
}

void UPBCameraControlComponent::AddMouseRotationInput(float AxisValue)
{
	TargetYaw = ClampYaw(TargetYaw + AxisValue * MouseRotateSensitivity);
}

void UPBCameraControlComponent::AddFreeLookInput(const FVector2D& Axis)
{
	// 현재 카메라 Yaw 기준으로 월드 방향 계산
	const FRotator YawRotation(0.0f, CurrentYaw, 0.0f);
	const FVector Forward = YawRotation.RotateVector(FVector::ForwardVector);
	const FVector Right = YawRotation.RotateVector(FVector::RightVector);

	const FVector Delta = (Forward * -Axis.Y + Right * -Axis.X) * FreeLookSensitivity;

	TargetCameraOffset += Delta;
	ClampCameraOffset();

	bIsResettingOffset = false;
}

// Tick
void UPBCameraControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateCamera(DeltaTime);
}

void UPBCameraControlComponent::UpdateCamera(float DeltaTime)
{
	if (!IsValid(CachedSpringArm))
	{
		return;
	}

	// 줌 보간
	CurrentZoomLevel = FMath::FInterpTo(CurrentZoomLevel, TargetZoomLevel, DeltaTime, ZoomInterpSpeed);

	const float NewArmLength = FMath::Lerp(MaxArmLength, MinArmLength, CurrentZoomLevel);
	CachedSpringArm->TargetArmLength = NewArmLength;

	// 구면 줌: Pitch 연동
	if (CurrentModeParams.bUseSphericalZoom)
	{
		const float NewPitch = FMath::Lerp(MaxPitch, MinPitch, CurrentZoomLevel);
		FRotator SpringArmRotation = CachedSpringArm->GetRelativeRotation();
		SpringArmRotation.Pitch = NewPitch;
		CachedSpringArm->SetRelativeRotation(SpringArmRotation);
	}

	// 회전 보간
	CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, RotationInterpSpeed);

	{
		FRotator SpringArmRotation = CachedSpringArm->GetRelativeRotation();
		SpringArmRotation.Yaw = CurrentYaw;
		CachedSpringArm->SetRelativeRotation(SpringArmRotation);
	}

	// 카메라 오프셋 보간
	CurrentCameraOffset = FMath::VInterpTo(CurrentCameraOffset, TargetCameraOffset, DeltaTime, CameraOffsetInterpSpeed);

	// 월드 스페이스 오프셋 적용
	if (IsValid(CachedSpringArm->GetAttachParent()))
	{
		const FVector AdjustedOffset(CurrentCameraOffset.X, CurrentCameraOffset.Y, 0.0f);
		CachedSpringArm->SetWorldLocation(
			CachedSpringArm->GetAttachParent()->GetComponentLocation() + AdjustedOffset
		);
	}

	// 리셋 보간 완료 확인
	if (bIsResettingOffset && CurrentCameraOffset.IsNearlyZero(1.0f))
	{
		CurrentCameraOffset = FVector::ZeroVector;
		bIsResettingOffset = false;
	}
}


// Utilities
float UPBCameraControlComponent::ClampYaw(float InYaw) const
{
	// -1은 무제한
	if (CurrentModeParams.RotateRangeMin < 0.0f && CurrentModeParams.RotateRangeMax < 0.0f)
	{
		return InYaw;
	}

	return FMath::Clamp(InYaw, CurrentModeParams.RotateRangeMin, CurrentModeParams.RotateRangeMax);
}

void UPBCameraControlComponent::ClampCameraOffset()
{
	if (TargetCameraOffset.Size() > MaxCameraOffsetDistance)
	{
		TargetCameraOffset = TargetCameraOffset.GetSafeNormal() * MaxCameraOffsetDistance;
	}
}
