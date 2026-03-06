// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "PBGameplayPlayerController.generated.h"

class UNiagaraSystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnViewDataPropertyChangedSiganture);

class UPBPathDisplayComponent;
class UPBCameraControlComponent;
class UInputMappingContext;
class UInputAction;
struct FPBCameraModeParams;

// 플레이어 컨트롤러 기반 클래스. 입력 처리, 플레이어 폰 제어 로직 담당.
UCLASS()
class PROJECTB3_API APBGameplayPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APBGameplayPlayerController();

	// 목표 위치까지의 NavPath를 쿼리하고 경로 시각화를 갱신
	UFUNCTION(BlueprintCallable, Category = "PathDisplay")
	void RequestNavPathDisplay(const FVector& TargetLocation);

	// 카메라 제어 컴포넌트 반환
	UPBCameraControlComponent* GetCameraControl() const { return CameraControlComponent; }

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/*~ APlayerController Interface ~*/
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	// 마우스 클릭 시 폰을 해당 위치로 이동 명령
	void OnMoveCommand(const FInputActionValue& Value);

	// 매 틱마다 커서 아래 위치로 경로 미리보기를 갱신
	void UpdateHoverPathDisplay();

	// 경로 포인트 배열에서 MaxMoveDistance 기준으로 실제 이동 목적지를 계산
	FVector CalculateClampedDestination(const TArray<FVector>& PathPoints) const;

	/*~ Camera Control ~*/
	void OnCameraZoom(const FInputActionValue& Value);
	void OnCameraRotate(const FInputActionValue& Value);
	void OnCameraMouseRotateStart(const FInputActionValue& Value);
	void OnCameraMouseRotateStop(const FInputActionValue& Value);
	void OnCameraFreeLookStart(const FInputActionValue& Value);
	void OnCameraFreeLookStop(const FInputActionValue& Value);
	void OnCameraReset(const FInputActionValue& Value);

	// Possess 시 SpringArm 참조를 컴포넌트에 전달
	void BindCameraToCharacter();

public:
	/*~ Input Settings ~*/

	// 기본 입력 매핑 컨텍스트
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// 카메라 입력 매핑 컨텍스트
	UPROPERTY(EditAnywhere, Category = "Input|Camera")
	TObjectPtr<UInputMappingContext> CameraMappingContext;

	// 이동 명령 입력 액션 (좌클릭)
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveCommandAction;

	// 카메라 줌 입력 액션 (마우스 휠)
	UPROPERTY(EditAnywhere, Category = "Input|Camera")
	TObjectPtr<UInputAction> CameraZoomAction;

	// 카메라 스텝 회전 입력 액션 (Q/E)
	UPROPERTY(EditAnywhere, Category = "Input|Camera")
	TObjectPtr<UInputAction> CameraRotateAction;

	// 카메라 마우스 회전 입력 액션 (중클릭 드래그)
	UPROPERTY(EditAnywhere, Category = "Input|Camera")
	TObjectPtr<UInputAction> CameraMouseRotateAction;

	// 카메라 프리 룩 입력 액션 (우클릭 드래그)
	UPROPERTY(EditAnywhere, Category = "Input|Camera")
	TObjectPtr<UInputAction> CameraFreeLookAction;

	// 카메라 리셋 입력 액션 (Space)
	UPROPERTY(EditAnywhere, Category = "Input|Camera")
	TObjectPtr<UInputAction> CameraResetAction;

	/*~ Movement Settings ~*/

	// 캐릭터의 최대 이동 가능 거리
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxMoveDistance = 600.0f;

	/*~ PathDisplay Settings ~*/

	// 경로 갱신을 트리거하는 최소 커서 이동 거리
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	float PathUpdateMinDistance = 20.0f;

	/*~ FX Settings ~*/
	UPROPERTY(EditAnywhere, Category = "Input")
	UNiagaraSystem* CursorVFX;

private:
	// 이동 경로 시각화 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBPathDisplayComponent> PathDisplayComponent;

	// 카메라 제어 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBCameraControlComponent> CameraControlComponent;

	// 마지막으로 경로를 갱신한 커서 위치
	FVector LastHoverLocation = FVector::ZeroVector;

	// 마우스 회전(중클릭) 홀드 여부
	bool bIsMouseRotationHeld = false;

	// 마우스 프리룩(우클릭/드래그 등) 홀드 여부
	bool bIsFreeLookHeld = false;
};
