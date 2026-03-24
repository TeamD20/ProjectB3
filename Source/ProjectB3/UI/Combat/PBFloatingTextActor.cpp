// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBFloatingTextActor.h"
#include "PBFloatingTextWidget.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/AbilitySystem/Payload/PBFloatingTextPayload.h"

APBFloatingTextActor::APBFloatingTextActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	WidgetComp->SetupAttachment(SceneRoot);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawAtDesiredSize(true);
}

void APBFloatingTextActor::InitWithPayload(UPBFloatingTextPayload* Payload, TSubclassOf<UPBFloatingTextWidget> WidgetClass)
{
	CachedPayload = Payload;
	CachedWidgetClass = WidgetClass;
	bInitialized = true;
}

void APBFloatingTextActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bInitialized || !IsValid(CachedPayload) || !CachedWidgetClass)
	{
		Destroy();
		return;
	}

	// 위젯 클래스 설정 및 생성
	WidgetComp->SetWidgetClass(CachedWidgetClass);
	WidgetComp->InitWidget();

	UPBFloatingTextWidget* FloatingWidget = Cast<UPBFloatingTextWidget>(WidgetComp->GetWidget());
	if (!IsValid(FloatingWidget))
	{
		Destroy();
		return;
	}

	// 페이로드 바인딩 및 애니메이션 재생
	FloatingWidget->SetPayload(CachedPayload);
	FloatingWidget->OnFloatingTextAnimationFinished.BindUObject(this, &APBFloatingTextActor::OnWidgetAnimationFinished);
	FloatingWidget->PlayShowAnimation();
}

void APBFloatingTextActor::SetFollowParams(AActor* InFollowTarget, float InOffset)
{
	FollowTarget = InFollowTarget;
	FollowOffset = InOffset;
}

void APBFloatingTextActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!FollowTarget.IsValid())
	{
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager))
	{
		return;
	}

	const FVector TargetLocation = FollowTarget->GetActorLocation();
	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CameraUp = FRotationMatrix(PC->PlayerCameraManager->GetCameraRotation()).GetUnitAxis(EAxis::Z);

	// 카메라 거리에 비례하여 오프셋 스케일링 — 줌 레벨에 관계없이 화면상 일정 간격 유지
	const float CameraDistance = FVector::Dist(CameraLocation, TargetLocation);
	static constexpr float ReferenceDistance = 1000.f;
	const float DistanceScale = CameraDistance / ReferenceDistance;

	const FVector NewLocation = TargetLocation + CameraUp * (FollowOffset * DistanceScale);
	SetActorLocation(NewLocation);
}

void APBFloatingTextActor::OnWidgetAnimationFinished()
{
	Destroy();
}
