// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "PBGameplayPlayerController.generated.h"

class UPathFollowingComponent;
class UPBDialogueManagerComponent;
class UNiagaraSystem;
class UPBPartyFollowSubsystem;
class UPBPathDisplayComponent;
class UPBCameraControlComponent;
class UPBTargetingComponent;
class UPBInteractorComponent;
class UPBTacticalCameraComponent;
class UPBWidgetBase;
class UInputMappingContext;
class UInputAction;
struct FPBCameraModeParams;
struct FPBTargetingRequest;
class UPBDefaultCursorWidget;
class UPBTargetingCursorWidget;
struct FPBAbilityTargetData;

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
	
	// 전술 카메라 컴포넌트 반환
	UPBTacticalCameraComponent* GetTacticalCameraComponent() const { return TacticalCameraComponent; }

	// 타겟팅 컴포넌트 반환
	UPBTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	// 상호작용 컴포넌트 반환
	UPBInteractorComponent* GetInteractorComponent() const { return InteractorComponent; }

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

	// 인벤토리 UI를 토글 (열기/닫기)
	UFUNCTION(BlueprintCallable)
	void ToggleInventory();
	
	// GameOver UI 표시
	void OnGameOver();

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/*~ APlayerController Interface ~*/
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	
	/*~ APBGameplayPlayerController Interface ~*/

	// 암전 상태를 유지한 뒤 페이드 인 처리
	void FadeInFromBlack();

	/*~ Camera Cutout ~*/
	// 카메라에서 파티원들을 향해 방해물을 검사하여 투명화 처리한다.
	void UpdateCameraCutout();

	// 특정 액터(투명화된 장애물 등)를 무시하고 커서 위치를 트레이스한다.
	bool GetCursorHitWithIgnoredActors(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& OutHitResult) const;
	
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

	// 인벤토리 토글 입력 이벤트 처리
	void OnToggleInventory(const FInputActionValue& Value);

	// MultiTarget 선택 변경 시 타겟팅 커서 게이지 갱신
	void OnTargetSelectionChanged(const FPBAbilityTargetData& TargetData);

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
	TObjectPtr<UInputAction> SelectCommandAction;

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

	// 인벤토리 토글 입력 액션 (I)
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	// 인벤토리 UI 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPBWidgetBase> InventoryWidgetClass;

	// 게임오버 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPBWidgetBase> GameOverWidgetClass;

	// 게임오버 시 재생될 사운드
	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> GameOverSound;
	
	// 커서 위젯 설정
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPBDefaultCursorWidget> DefaultCursorWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UPBTargetingCursorWidget> TargetingCursorWidgetClass;

	// 경로 갱신을 트리거하는 최소 커서 이동 거리
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	float PathUpdateMinDistance = 10.0f;

	/*~ FX Settings ~*/
	UPROPERTY(EditAnywhere, Category = "Input")
	UNiagaraSystem* CursorVFX;

	// 맵 진입 시 페이드 인 소요 시간 (초)
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float FadeInDuration = 1.0f;

	// 맵 진입 후 페이드 인 시작 전 암전 유지 시간 (초)
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float FadeInInitialDelay = 2.5f;

	// 투명화 트레이스를 수행할 주기 (초)
	UPROPERTY(EditDefaultsOnly, Category = "Camera Cutout")
	float CutoutTraceInterval = 0.1f;
	
private:
	// 경로 탐색 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPathFollowingComponent> PathFollowingComponent;
	
	// 이동 경로 시각화 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBPathDisplayComponent> PathDisplayComponent;

	// 카메라 제어 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBCameraControlComponent> CameraControlComponent;

	// 타겟팅 세션 수명 관리 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBTargetingComponent> TargetingComponent;

	// 상호작용 포커스 및 위임 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBInteractorComponent> InteractorComponent;

	// 전술 카메라 제어 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBTacticalCameraComponent> TacticalCameraComponent;

	// 대화 매니저 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBDialogueManagerComponent> DialogueManagerComponent;
	
	// 현재 PC 입력 모드
	EPBPlayerControllerMode CurrentMode = EPBPlayerControllerMode::None;

	// 마지막으로 경로를 갱신한 커서 위치
	FVector LastHoverLocation = FVector::ZeroVector;

	// 마우스 회전(중클릭) 홀드 여부
	bool bIsMouseRotationHeld = false;

	// 마우스 프리룩(우클릭/드래그 등) 홀드 여부
	bool bIsFreeLookHeld = false;
	
	// 인벤토리 표시 여부
	bool bIsInventoryOpen = false;

	// 카메라 컷아웃 타이머 핸들
	FTimerHandle CutoutTimerHandle;

	// FreeMovement 중 이전 Tick에 리더가 이동 중이었는지 여부 (정지 감지용)
	bool bWasLeaderMoving = false;

	// 리더 정지 확정 디바운스 타이머 (0.1초 유지 후 서브시스템에 통보)
	FTimerHandle LeaderStopDebounceTimer;

	struct FPBMeshFadeState 
	{
		bool bHitByMember[4] = {false, false, false, false};
	};

	// 현재 투명화 상태인 메시 컴포넌트 목록 (파티원별 히트 상태 추적)
	TMap<TWeakObjectPtr<UMeshComponent>, FPBMeshFadeState> FadedMeshes;

	// 현재 투명화 상태인 장애물 액터 목록 (트레이스 무시용)
	TSet<TWeakObjectPtr<AActor>> FadedActors;
	
	UPROPERTY()
	TObjectPtr<UPBDefaultCursorWidget> DefaultCursorWidget;

	UPROPERTY()
	TObjectPtr<UPBTargetingCursorWidget> TargetingCursorWidget;
};
