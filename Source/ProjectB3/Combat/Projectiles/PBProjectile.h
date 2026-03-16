// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

// 투사체 기반 클래스. 이동·충돌·최대 사거리(XY 평면 기준) 제한을 공통 처리.
UCLASS(Abstract)
class PROJECTB3_API APBProjectile : public AActor
{
	GENERATED_BODY()

public:
	APBProjectile();

	// 방향 벡터 기반 투사체 발사. LaunchOrigin2D를 현재 XY 위치로 기록.
	void Launch(const FVector& Direction);

protected:
	/*~ AActor Interface ~*/
	virtual void Tick(float DeltaSeconds) override;

	// 최대 사거리 초과 시 호출. 기본 동작: Destroy. 서브클래스에서 오버라이드 가능.
	virtual void OnRangeExceeded();

	// 충돌 발생 시 호출. 서브클래스에서 오버라이드하여 데미지 적용 등을 처리.
	virtual void OnProjectileOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult);

private:
	UFUNCTION()
	void HandleOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
protected:
	// 충돌 처리용 구체 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	// 투사체 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// 비주얼 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 초기/최대 이동 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float InitialSpeed = 3000.f;

	// 최대 이동 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float MaxSpeed = 3000.f;

	// 최대 비행 거리 (XY 평면 기준, cm). 0이면 무제한.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float MaxRange = 0.f;

private:

	// Launch() 호출 시점의 XY 위치 (사거리 계산 기준점)
	FVector2D LaunchOrigin2D;

	// Launch() 호출 여부 (Tick 사거리 검사 활성화 기준)
	bool bLaunched = false;
};
