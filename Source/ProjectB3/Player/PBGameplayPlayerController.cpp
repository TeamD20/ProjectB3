// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayPlayerController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "NiagaraFunctionLibrary.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "ProjectB3/Camera/PBCameraControlComponent.h"
#include "ProjectB3/Characters/PBPlayerCharacter.h"
#include "ProjectB3/NavigationSystem/PBPathDisplayComponent.h"

APBGameplayPlayerController::APBGameplayPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	PathDisplayComponent = CreateDefaultSubobject<UPBPathDisplayComponent>(TEXT("PathDisplayComponent"));
	CameraControlComponent = CreateDefaultSubobject<UPBCameraControlComponent>(TEXT("CameraControlComponent"));
}

void APBGameplayPlayerController::BeginPlay()
{
	// ValidateProperties가 올바른 값으로 실행되도록 Super 호출 전에 동기화
	PathDisplayComponent->SetMaxMoveDistance(MaxMoveDistance);

	Super::BeginPlay();

	bShowMouseCursor = true;

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
	
	UpdateHoverPathDisplay();
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
		EnhancedInput->BindAction(MoveCommandAction, ETriggerEvent::Started, this, &APBGameplayPlayerController::OnMoveCommand);
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

// ============================================================================
// Camera Control
// ============================================================================

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

// ============================================================================
// Movement & Path Display
// ============================================================================

void APBGameplayPlayerController::OnMoveCommand(const FInputActionValue& Value)
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

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!IsValid(NavSys))
	{
		return;
	}

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(
		GetWorld(), MyPawn->GetActorLocation(), HitResult.Location);

	if (!IsValid(NavPath) || !NavPath->IsValid())
	{
		return;
	}

	const FVector Destination = CalculateClampedDestination(NavPath->PathPoints);
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Destination);
	PathDisplayComponent->ClearPath();

	if (CursorVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CursorVFX, HitResult.Location, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}
}

FVector APBGameplayPlayerController::CalculateClampedDestination(const TArray<FVector>& PathPoints) const
{
	if (PathPoints.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	if (MaxMoveDistance <= -1.0f)
	{
		return PathPoints.Last();
	}

	const float MaxDist = MaxMoveDistance;
	float AccumulatedDist = 0.0f;

	for (int32 i = 1; i < PathPoints.Num(); ++i)
	{
		const float SegDist = FVector::Dist(PathPoints[i - 1], PathPoints[i]);

		if (AccumulatedDist + SegDist >= MaxDist)
		{
			const float Remaining = MaxDist - AccumulatedDist;
			const float T = (SegDist > 0.0f) ? (Remaining / SegDist) : 0.0f;
			return FMath::Lerp(PathPoints[i - 1], PathPoints[i], T);
		}

		AccumulatedDist += SegDist;
	}

	return PathPoints.Last();
}

void APBGameplayPlayerController::UpdateHoverPathDisplay()
{
	if (!PathDisplayComponent->IsPathDisplayEnabled())
	{
		return;
	}

	FHitResult HitResult;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
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
