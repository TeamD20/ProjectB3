// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "PBCameraTypes.h"
#include "PBCameraControlComponent.generated.h"

class USpringArmComponent;

// 카메라 조작 로직을 담당하는 컴포넌트.
// 줌/회전/오프셋 상태를 소유하고, SpringArm을 매 프레임 갱신한다.
// PlayerController에 부착하여 캐릭터 전환에도 카메라 상태를 유지한다.
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class PROJECTB3_API UPBCameraControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBCameraControlComponent();

	// SpringArm 참조를 설정 (Possess 시 호출)
	void SetSpringArm(USpringArmComponent* InSpringArm);

	// 현재 카메라 모드 파라미터를 교체
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetModeParams(const FPBCameraModeParams& NewParams);

	// 카메라 오프셋을 캐릭터 중심으로 리셋 (보간)
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void ResetOffset();

	/*~ 입력 수신 인터페이스 (PlayerController에서 호출) ~*/

	// 줌 입력 (마우스 휠 축 값)
	void AddZoomInput(float AxisValue);

	// 스텝 회전 입력 (Q/E 축 값: -1 또는 +1)
	void AddRotationStepInput(float AxisValue);

	// 마우스 자유 회전 입력 (중클릭 드래그 축 값)
	void AddMouseRotationInput(float AxisValue);

	// 프리 룩 입력 (우클릭 드래그 2D 축 값)
	void AddFreeLookInput(const FVector2D& Axis);

protected:
	/*~ UActorComponent Interface ~*/
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// 줌/회전/오프셋 보간 및 SpringArm 갱신
	void UpdateCamera(float DeltaTime);

	// TargetYaw를 현재 모드의 RotateRange로 클램프
	float ClampYaw(float InYaw) const;

	// 카메라 오프셋을 최대 거리로 클램프
	void ClampCameraOffset();

public:
	/*~ Camera Zoom Settings ~*/

	// 최대 줌아웃 시 SpringArm 길이
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float MaxArmLength = 2500.0f;

	// 최대 줌인 시 SpringArm 길이
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float MinArmLength = 500.0f;

	// 줌아웃 시 Pitch (탑다운)
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float MaxPitch = -70.0f;

	// 줌인 시 Pitch (수평에 가까움)
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom")
	float MinPitch = -35.0f;

	// 줌 단계당 변화량
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float ZoomStep = 0.08f;

	// 줌 보간 속도
	UPROPERTY(EditAnywhere, Category = "Camera|Zoom", meta = (ClampMin = "1.0"))
	float ZoomInterpSpeed = 8.0f;

	/*~ Camera Rotation Settings ~*/

	// Q/E 스텝 회전 각도
	UPROPERTY(EditAnywhere, Category = "Camera|Rotation")
	float RotationStepDegrees = 45.0f;

	// 회전 보간 속도
	UPROPERTY(EditAnywhere, Category = "Camera|Rotation", meta = (ClampMin = "1.0"))
	float RotationInterpSpeed = 8.0f;

	// 마우스 회전 감도
	UPROPERTY(EditAnywhere, Category = "Camera|Rotation", meta = (ClampMin = "0.1"))
	float MouseRotateSensitivity = 1.0f;

	/*~ Camera Move Settings ~*/

	// 카메라 오프셋 최대 거리
	UPROPERTY(EditAnywhere, Category = "Camera|Move")
	float MaxCameraOffsetDistance = 2000.0f;

	// 카메라 이동 속도
	UPROPERTY(EditAnywhere, Category = "Camera|Move")
	float CameraMoveSpeed = 1500.0f;

	// 카메라 오프셋 보간 속도
	UPROPERTY(EditAnywhere, Category = "Camera|Move", meta = (ClampMin = "1.0"))
	float CameraOffsetInterpSpeed = 6.0f;

	// 마우스 프리 룩 감도 (이동 거리 비율)
	UPROPERTY(EditAnywhere, Category = "Camera|Move", meta = (ClampMin = "0.1"))
	float FreeLookSensitivity = 25.0f;

protected:
	// 현재 카메라 모드 파라미터
	UPROPERTY(EditAnywhere, Category = "Camera")
	FPBCameraModeParams CurrentModeParams;

private:
	// 캐싱된 SpringArm 참조
	UPROPERTY()
	TObjectPtr<USpringArmComponent> CachedSpringArm;

	/*~ Camera State (캐릭터 전환에도 유지) ~*/

	// 현재/목표 줌 레벨 [0.0 ~ 1.0]
	float CurrentZoomLevel = 0.3f;
	float TargetZoomLevel = 0.3f;

	// 현재/목표 Yaw (도)
	float CurrentYaw = 0.0f;
	float TargetYaw = 0.0f;

	// 현재/목표 카메라 오프셋
	FVector CurrentCameraOffset = FVector::ZeroVector;
	FVector TargetCameraOffset = FVector::ZeroVector;

	// 카메라 리셋 보간 중 플래그
	bool bIsResettingOffset = false;
};
