// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

APBProjectile::APBProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	SetRootComponent(CollisionComponent);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	// Launch() 호출 전까지 이동하지 않도록 비활성 상태로 생성
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleOverlap);
}

void APBProjectile::Launch(const FVector& Direction)
{
	// 사거리 계산 기준점 기록
	LaunchOrigin2D = FVector2D(GetActorLocation().X, GetActorLocation().Y);
	bLaunched = true;

	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->Velocity = Direction.GetSafeNormal() * InitialSpeed;
	ProjectileMovement->Activate();
}

void APBProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bLaunched || MaxRange <= 0.f)
	{
		return;
	}

	// XY 평면 기준 이동 거리가 MaxRange를 초과하면 소멸 처리
	const FVector2D CurrentXY(GetActorLocation().X, GetActorLocation().Y);
	if ((CurrentXY - LaunchOrigin2D).Size() >= MaxRange)
	{
		OnRangeExceeded();
	}
}

void APBProjectile::OnRangeExceeded()
{
	Destroy();
}

void APBProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}


void APBProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	OnProjectileOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex,bFromSweep,SweepResult);
}