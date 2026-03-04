// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayPlayerController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "ProjectB3/NavigationSystem/PBPathDisplayComponent.h"

APBGameplayPlayerController::APBGameplayPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	PathDisplayComponent = CreateDefaultSubobject<UPBPathDisplayComponent>(TEXT("PathDisplayComponent"));
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
	}

	PathDisplayComponent->SetPathDisplayEnabled(true);
}

void APBGameplayPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
		EnhancedInput->BindAction(MoveCommandAction, ETriggerEvent::Triggered, this, &APBGameplayPlayerController::OnMoveCommandTriggered);
	}
}

void APBGameplayPlayerController::OnMoveCommandTriggered(const FInputActionValue& Value)
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
}

FVector APBGameplayPlayerController::CalculateClampedDestination(const TArray<FVector>& PathPoints) const
{
	if (PathPoints.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	const float MaxDist = MaxMoveDistance;
	float AccumulatedDist = 0.0f;

	for (int32 i = 1; i < PathPoints.Num(); ++i)
	{
		const float SegDist = FVector::Dist(PathPoints[i - 1], PathPoints[i]); // 두 점 사이의 거리

		if (AccumulatedDist + SegDist >= MaxDist)
		{
			// MaxMoveDistance에 해당하는 지점을 선형 보간으로 계산
			const float Remaining = MaxDist - AccumulatedDist;
			const float T = (SegDist > 0.0f) ? (Remaining / SegDist) : 0.0f;
			return FMath::Lerp(PathPoints[i - 1], PathPoints[i], T);
		}

		AccumulatedDist += SegDist;
	}

	// 전체 경로가 MaxMoveDistance보다 짧으면 마지막 지점 (목적지) 그대로 반환
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

	PathDisplayComponent->DisplayPath(NavPath->PathPoints,true);
}
