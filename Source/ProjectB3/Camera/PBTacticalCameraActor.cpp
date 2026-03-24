// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTacticalCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProjectB3/Utils/PBGameplayStatics.h"

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
	// 추적 해제 시 컷아웃도 즉시 복구
	TrackingTarget.Reset();
	UpdateCameraCutout();
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

	UpdateCameraCutout();
}

void APBTacticalCameraActor::UpdateCameraCutout()
{
	if (!TrackingTarget.IsValid())
	{
		// 추적 대상이 없으면 기존 컷아웃 해제
		for (const TWeakObjectPtr<UMeshComponent>& WeakMesh : FadedMeshes)
		{
			if (!WeakMesh.IsValid())
			{
				continue;
			}

			UMeshComponent* Mesh = WeakMesh.Get();
			int32 NumMaterials = Mesh->GetNumMaterials();
			for (int32 i = 0; i < NumMaterials; ++i)
			{
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i));
				if (IsValid(MID))
				{
					MID->SetScalarParameterValue(TEXT("TraceFadeAmount3"), 0.0f);
				}
			}
		}
		FadedMeshes.Empty();
		return;
	}

	FVector CameraLocation = GetActorLocation();
	FVector TargetLocation = TrackingTarget->GetActorLocation();

	FCollisionQueryParams QueryParams(TEXT("TacticalCameraCutout"), false, this);
	QueryParams.AddIgnoredActor(TrackingTarget.Get());

	TArray<FHitResult> HitResults;
	bool bHit = GetWorld()->LineTraceMultiByChannel(
		HitResults,
		CameraLocation,
		TargetLocation,
		ECC_Visibility,
		QueryParams
	);

	TSet<TWeakObjectPtr<UMeshComponent>> CurrentHits;

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			if (!IsValid(HitActor))
			{
				continue;
			}

			TArray<UMeshComponent*> HitMeshes;
			UPBGameplayStatics::GetAllMeshComponents(HitActor, HitMeshes);

			for (UMeshComponent* Mesh : HitMeshes)
			{
				if (!IsValid(Mesh))
				{
					continue;
				}

				CurrentHits.Add(Mesh);

				int32 NumMaterials = Mesh->GetNumMaterials();
				for (int32 i = 0; i < NumMaterials; ++i)
				{
					UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i));
					if (IsValid(MID))
					{
						MID->SetVectorParameterValue(TEXT("HitLocation3"), Hit.ImpactPoint);
						MID->SetScalarParameterValue(TEXT("TraceFadeAmount3"), 1.0f);
					}
					else
					{
						Mesh->CreateAndSetMaterialInstanceDynamic(i);
					}
				}
			}
		}
	}

	// 이전 프레임에 페이드 중이었으나 현재 히트되지 않은 메시 복구
	for (const TWeakObjectPtr<UMeshComponent>& WeakMesh : FadedMeshes)
	{
		if (!WeakMesh.IsValid())
		{
			continue;
		}

		if (CurrentHits.Contains(WeakMesh))
		{
			continue;
		}

		UMeshComponent* Mesh = WeakMesh.Get();
		int32 NumMaterials = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; ++i)
		{
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(i));
			if (IsValid(MID))
			{
				MID->SetScalarParameterValue(TEXT("TraceFadeAmount3"), 0.0f);
			}
		}
	}

	FadedMeshes = CurrentHits;
}
