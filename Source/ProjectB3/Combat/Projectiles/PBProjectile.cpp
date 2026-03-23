// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "PBProjectileUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

APBProjectile::APBProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(CollisionComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APBProjectile::InitProjectile(const FGameplayEffectSpecHandle& InDamageSpec, UAbilitySystemComponent* InSourceASC,
	AActor* InTargetActor)
{
	DamageSpecHandle = InDamageSpec;
	SourceASC = InSourceASC;
	TargetActor = InTargetActor;
}

void APBProjectile::Launch(const FVector& TargetLocation)
{
	BezierP0 = GetActorLocation();
	BezierP2 = TargetLocation;
	BezierP1 = PBProjectileUtils::CalcMidControlPoint(
		BezierP0, BezierP2, ArcHeightRatio, MinArcHeight, MaxArcHeight);

	// 비행 시간 결정: 수동 설정값 우선, 미설정 시 거리/속도 기반 자동 계산
	if (FlightDuration > 0.f)
	{
		ActiveFlightDuration = FlightDuration;
	}
	else
	{
		const float Dist = FVector::Dist(BezierP0, BezierP2);
		ActiveFlightDuration = Dist / InitialSpeed;
	}

	Alpha = 0.f;
	bLaunched = true;
}

void APBProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bLaunched)
	{
		return;
	}

	Alpha += DeltaSeconds / ActiveFlightDuration;

	if (Alpha >= 1.f)
	{
		Alpha = 1.f;
		SetActorLocation(BezierP2);
		InternalOnArrived();
		OnProjectileResolved.ExecuteIfBound(TargetActor.Get());
		return;
	}

	const FVector Pos = PBProjectileUtils::BezierPoint(BezierP0, BezierP1, BezierP2, Alpha);
	const FVector Tangent = PBProjectileUtils::BezierTangent(BezierP0, BezierP1, BezierP2, Alpha);

	SetActorLocationAndRotation(Pos, Tangent.ToOrientationQuat());
}

void APBProjectile::OnArrived()
{
	// 타겟에 데미지 적용
	if (TargetActor.IsValid() && SourceASC.IsValid() && DamageSpecHandle.IsValid())
	{
		if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(TargetActor.Get()))
		{
			UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent();
			if (IsValid(TargetASC))
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
			}
		}
	}
	Destroy();
}

void APBProjectile::InternalOnArrived()
{
	K2_OnArrived();
}

void APBProjectile::K2_OnArrived_Implementation()
{
	OnArrived();
}
