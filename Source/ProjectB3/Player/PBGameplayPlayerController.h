// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "PBGameplayPlayerController.generated.h"

class UNiagaraSystem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnViewDataPropertyChangedSiganture);

class UPBPathDisplayComponent;
class UPBCameraControlComponent;
class UPBTargetingComponent;
class UInputMappingContext;
class UInputAction;
struct FPBCameraModeParams;
struct FPBTargetingRequest;

/** PC 입력 처리 모드 */
UENUM(BlueprintType)
enum class EPBPlayerControllerMode : uint8
{
	// 입력 비활성 상태
	None,
	
	// 자유 이동 모드 (거리 제한 없음, PC 직접 제어)
	FreeMovement,
	
	// 턴 기반 이동 모드 (이동력 제한)
	TurnMovement,

	// 이동 실행 중 (MoveToLocation Task 동작 중)
	Moving,

	// 어빌리티 타겟팅 모드
	Targeting,
};

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

	// 타겟팅 컴포넌트 반환
	UPBTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	// 현재 컨트롤러 모드 조회
	EPBPlayerControllerMode GetControllerMode() const { return CurrentMode; }

	// PathDisplay의 이동 범위를 갱신 (이동 어빌리티 활성화 시 호출)
	void SetPathDisplayMovementRange(float Range);

	// PathDisplay 초기화
	void ClearPathDisplay();

	// 이동 시작 시 확정 경로를 전달하고 Moving 모드로 진입
	void BeginMoving(const TArray<FVector>& PathPoints);

	// 이동 종료 시 PathDisplay 추적 종료 (모드 전환은 호출부 담당)
	void EndMoving();
	
	// 현재 모드를 종료하고 해당 모드로 전환
	void SetControllerMode(EPBPlayerControllerMode NewMode);

	// 타겟팅 모드 진입 요청
	void EnterTargetingMode(const FPBTargetingRequest& Request);

	// 현재 모드를 None으로 종료
	void ExitCurrentMode();

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/*~ APlayerController Interface ~*/
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

private:
	// 마우스 좌클릭 — 모드에 따라 이동 명령 또는 타겟 선택/확정 처리
	void OnSelectCommand(const FInputActionValue& Value);

	// 마우스 우클릭 — Targeting 모드에서만 동작
	void OnRightClick(const FInputActionValue& Value);

	// 매 틱마다 커서 아래 위치로 경로 미리보기를 갱신
	void UpdateHoverPathDisplay(const FHitResult& HitResult);

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

	// 우클릭 입력 액션
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> RightClickAction;

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

	// 타겟팅 세션 수명 관리 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBTargetingComponent> TargetingComponent;

	// 현재 PC 입력 모드
	EPBPlayerControllerMode CurrentMode = EPBPlayerControllerMode::None;

	// 마지막으로 경로를 갱신한 커서 위치
	FVector LastHoverLocation = FVector::ZeroVector;

	// 마우스 회전(중클릭) 홀드 여부
	bool bIsMouseRotationHeld = false;

	// 마우스 프리룩(우클릭/드래그 등) 홀드 여부
	bool bIsFreeLookHeld = false;
};
