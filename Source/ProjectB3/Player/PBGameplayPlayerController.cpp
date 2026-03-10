// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayPlayerController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "PBTargetingComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/Camera/PBCameraControlComponent.h"
#include "ProjectB3/Characters/PBPlayerCharacter.h"
#include "ProjectB3/NavigationSystem/PBPathDisplayComponent.h"

APBGameplayPlayerController::APBGameplayPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	CheatClass = UPBPlayerCheatManager::StaticClass();

	CameraControlComponent = CreateDefaultSubobject<UPBCameraControlComponent>(TEXT("CameraControlComponent"));
	PathDisplayComponent = CreateDefaultSubobject<UPBPathDisplayComponent>(TEXT("PathDisplayComponent"));
	TargetingComponent = CreateDefaultSubobject<UPBTargetingComponent>(TEXT("TargetingComponent"));
}

void APBGameplayPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Enhanced Input 매핑 컨텍스트 등록
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (IsValid(DefaultMappingContext))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		if (IsValid(CameraMappingContext))
		{
			Subsystem->AddMappingContext(CameraMappingContext, 1);
		}
	}
	
	// 기본 마우스 표시
	bShowMouseCursor = true;
	
	// 초기 ControlMode 설정
	SetControllerMode(EPBPlayerControllerMode::FreeMovement);
	
	PathDisplayComponent->SetPathDisplayEnabled(true);

	BindCameraToCharacter();
}

void APBGameplayPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float MouseX = 0.0f;
	float MouseY = 0.0f;

	if (bIsMouseRotationHeld || bIsFreeLookHeld)
	{
		GetInputMouseDelta(MouseX, MouseY);
	}

	if (bIsMouseRotationHeld && FMath::Abs(MouseX) > 0.0f)
	{
		CameraControlComponent->AddMouseRotationInput(MouseX);
	}

	if (bIsFreeLookHeld && (FMath::Abs(MouseX) > 0.0f || FMath::Abs(MouseY) > 0.0f))
	{
		CameraControlComponent->AddFreeLookInput(FVector2D(MouseX, MouseY));
	}
	// 단일 라인트레이스 결과를 경로 표시와 타겟팅이 공유
	FHitResult CursorHit;
	const bool bGotHit = GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);

	if (bGotHit)
	{
		if (CurrentMode == EPBPlayerControllerMode::Movement || CurrentMode == EPBPlayerControllerMode::FreeMovement)
		{
			UpdateHoverPathDisplay(CursorHit);
		}
		if (CurrentMode == EPBPlayerControllerMode::Targeting)
		{
			TargetingComponent->UpdateTargetingFromHit(CursorHit);
		}
	}
}

void APBGameplayPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInput))
	{
		return;
	}

	if (IsValid(MoveCommandAction))
	{
		EnhancedInput->BindAction(MoveCommandAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnSelectCommand);
	}
	if (IsValid(CameraZoomAction))
	{
		EnhancedInput->BindAction(CameraZoomAction, ETriggerEvent::Triggered, this, &APBGameplayPlayerController::OnCameraZoom);
	}
	if (IsValid(CameraRotateAction))
	{
		EnhancedInput->BindAction(CameraRotateAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnCameraRotate);
	}
	if (IsValid(CameraMouseRotateAction))
	{
		EnhancedInput->BindAction(CameraMouseRotateAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnCameraMouseRotateStart);
		EnhancedInput->BindAction(CameraMouseRotateAction, ETriggerEvent::Completed, this, &APBGameplayPlayerController::OnCameraMouseRotateStop);
		EnhancedInput->BindAction(CameraMouseRotateAction, ETriggerEvent::Canceled, this, &APBGameplayPlayerController::OnCameraMouseRotateStop);
	}
	if (IsValid(CameraFreeLookAction))
	{
		EnhancedInput->BindAction(CameraFreeLookAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnCameraFreeLookStart);
		EnhancedInput->BindAction(CameraFreeLookAction, ETriggerEvent::Completed, this, &APBGameplayPlayerController::OnCameraFreeLookStop);
		EnhancedInput->BindAction(CameraFreeLookAction, ETriggerEvent::Canceled, this, &APBGameplayPlayerController::OnCameraFreeLookStop);
	}
	if (IsValid(CameraResetAction))
	{
		EnhancedInput->BindAction(CameraResetAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnCameraReset);
	}
}

void APBGameplayPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	BindCameraToCharacter();

	// 캐릭터 전환 시 카메라 오프셋을 새 캐릭터 중심으로 리셋
	CameraControlComponent->ResetOffset();
}

void APBGameplayPlayerController::OnCameraZoom(const FInputActionValue& Value)
{
	CameraControlComponent->AddZoomInput(Value.Get<float>());
}

void APBGameplayPlayerController::OnCameraRotate(const FInputActionValue& Value)
{
	CameraControlComponent->AddRotationStepInput(Value.Get<float>());
}

void APBGameplayPlayerController::OnCameraMouseRotateStart(const FInputActionValue& Value)
{
	bIsMouseRotationHeld = true;
}

void APBGameplayPlayerController::OnCameraMouseRotateStop(const FInputActionValue& Value)
{
	bIsMouseRotationHeld = false;
}

void APBGameplayPlayerController::OnCameraFreeLookStart(const FInputActionValue& Value)
{
	bIsFreeLookHeld = true;
}

void APBGameplayPlayerController::OnCameraFreeLookStop(const FInputActionValue& Value)
{
	bIsFreeLookHeld = false;
}

void APBGameplayPlayerController::OnCameraReset(const FInputActionValue& Value)
{
	CameraControlComponent->ResetOffset();
}

void APBGameplayPlayerController::BindCameraToCharacter()
{
	APBPlayerCharacter* PlayerChar = Cast<APBPlayerCharacter>(GetPawn());
	if (IsValid(PlayerChar))
	{
		CameraControlComponent->SetSpringArm(PlayerChar->GetSpringArm());
	}
}

void APBGameplayPlayerController::SetControllerMode(EPBPlayerControllerMode NewMode)
{
	if (CurrentMode == NewMode)
	{
		return;
	}

	// 이전 모드 종료 처리
	if (CurrentMode == EPBPlayerControllerMode::Targeting)
	{
		if (TargetingComponent->IsTargetingActive())
		{
			TargetingComponent->ExitTargetingMode();
		}
	}

	CurrentMode = NewMode;

	PathDisplayComponent->ClearPath();
	
	// FreeMovement 진입 시 PathDisplay 거리 제한 해제
	if (NewMode == EPBPlayerControllerMode::FreeMovement)
	{
		PathDisplayComponent->SetMaxMoveDistance(-1.0f);
	}
}

void APBGameplayPlayerController::SetPathDisplayMovementRange(float Range)
{
	PathDisplayComponent->SetMaxMoveDistance(Range);
}

void APBGameplayPlayerController::ClearPathDisplay()
{
	PathDisplayComponent->ClearPath();
}

void APBGameplayPlayerController::EnterTargetingMode(const FPBTargetingRequest& Request)
{
	// TargetingComponent에 세션 진입 요청 후 모드 전환
	TargetingComponent->EnterTargetingMode(Request);
	SetControllerMode(EPBPlayerControllerMode::Targeting);
}

void APBGameplayPlayerController::ExitCurrentMode()
{
	SetControllerMode(EPBPlayerControllerMode::None);
}

void APBGameplayPlayerController::OnSelectCommand(const FInputActionValue& Value)
{
	switch (CurrentMode)
	{
	case EPBPlayerControllerMode::Targeting:
		// 타겟팅 모드: MultiTarget는 후보 추가, 그 외는 즉시 확정
		if (TargetingComponent->IsMultiTargetMode())
		{
			TargetingComponent->AddTargetSelection();
		}
		else
		{
			TargetingComponent->ConfirmTarget();
		}
		return;

	case EPBPlayerControllerMode::Movement:
	{
		FHitResult HitResult;
		if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			return;
		}

		APawn* MyPawn = GetPawn();
		if (!IsValid(MyPawn))
		{
			return;
		}

		// 이동 Payload 구성 후 이동 어빌리티에 이벤트 전송
		UPBTargetPayload* MovePayload = NewObject<UPBTargetPayload>(this);
		MovePayload->TargetData.TargetingMode = EPBTargetingMode::Location;
		MovePayload->TargetData.TargetLocations = { HitResult.Location };

		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyPawn);
		if (IsValid(ASC))
		{
			FGameplayEventData EventData;
			EventData.OptionalObject = MovePayload;
				ASC->HandleGameplayEvent(PBGameplayTags::Event_Movement_MoveCommand, &EventData);
		}

		PathDisplayComponent->ClearPath();
		if (IsValid(CursorVFX))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CursorVFX, HitResult.Location, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
		}
		return;
	}
	case EPBPlayerControllerMode::FreeMovement:
	{
		FHitResult HitResult;
		if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			return;
		}

		APawn* MyPawn = GetPawn();
		if (!IsValid(MyPawn))
		{
			return;
		}

		// 거리 제한 없이 PC가 직접 이동 명령
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitResult.Location);

		PathDisplayComponent->ClearPath();
		if (IsValid(CursorVFX))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CursorVFX, HitResult.Location, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
		}
		return;
	}
	case EPBPlayerControllerMode::None:
	default:
		return;
	}
}

void APBGameplayPlayerController::OnRightClick(const FInputActionValue& Value)
{
	// Targeting 모드 처리. MultiTarget: 마지막 후보 제거.
	if (CurrentMode == EPBPlayerControllerMode::Targeting)
	{
		if (TargetingComponent->IsMultiTargetMode())
		{
			TargetingComponent->RemoveLastTarget();
		}
		else
		{
			TargetingComponent->CancelTargeting();
		}
	}
}

void APBGameplayPlayerController::UpdateHoverPathDisplay(const FHitResult& HitResult)
{
	if (!PathDisplayComponent->IsPathDisplayEnabled())
	{
		return;
	}

	const FVector HoverLocation = HitResult.ImpactPoint;
	if (FVector::Dist(HoverLocation, LastHoverLocation) < PathUpdateMinDistance)
	{
		return;
	}

	LastHoverLocation = HoverLocation;
	RequestNavPathDisplay(HoverLocation);
}

void APBGameplayPlayerController::RequestNavPathDisplay(const FVector& TargetLocation)
{
	APawn* MyPawn = GetPawn();
	if (!IsValid(MyPawn))
	{
		return;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!IsValid(NavSys))
	{
		return;
	}

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(
		GetWorld(), MyPawn->GetActorLocation(), TargetLocation);

	if (!IsValid(NavPath) || !NavPath->IsValid())
	{
		PathDisplayComponent->ClearPath();
		return;
	}

	PathDisplayComponent->DisplayPath(NavPath->PathPoints, true);
}
