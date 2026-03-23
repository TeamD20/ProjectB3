// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBProjectile.generated.h"

class USphereComponent;

// 투사체 기반 클래스. Bezier 곡선 보간으로 발사 지점 → 타겟 위치까지 이동.
UCLASS(Abstract)
class PROJECTB3_API APBProjectile : public AActor
{
	GENERATED_BODY()

public:
	APBProjectile();

	// 타겟 위치를 향해 Bezier 곡선 비행 시작.
	void Launch(const FVector& TargetLocation);

	// Arc 파라미터 접근자 (경로 프리뷰용)
	float GetArcHeightRatio() const { return ArcHeightRatio; }
	float GetMinArcHeight() const { return MinArcHeight; }
	float GetMaxArcHeight() const { return MaxArcHeight; }

protected:
	/*~ AActor Interface ~*/
	virtual void Tick(float DeltaSeconds) override;

	// 타겟 위치 도착 시 호출. 서브클래스에서 오버라이드하여 데미지 적용 등을 처리.
	virtual void OnArrived();

protected:
	// 충돌 처리용 구체 콜리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	// 비주얼 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	// 초기 이동 속도 (cm/s). FlightDuration 미설정(0) 시 비행 시간 자동 계산에 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float InitialSpeed = 900.f;

	// 비행 시간 (초). 0이면 InitialSpeed 기반 자동 계산.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float FlightDuration = 0.f;

	// 호 높이 비율 (수평 거리 대비)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Arc")
	float ArcHeightRatio = 0.3f;

	// 최소 호 높이 (cm) — 근거리에서도 최소한의 곡선 보장
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Arc", meta = (ClampMin = "0.0"))
	float MinArcHeight = 50.f;

	// 최대 호 높이 (cm) — 과도한 곡선 방지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Arc", meta = (ClampMin = "0.0"))
	float MaxArcHeight = 500.f;

private:
	// Bezier 제어점
	FVector BezierP0;
	FVector BezierP1;
	FVector BezierP2;

	// 보간 진행도 [0, 1]
	float Alpha = 0.f;

	// 실제 비행에 사용되는 비행 시간 (Launch 시점에 결정)
	float ActiveFlightDuration = 0.f;

	// Launch() 호출 여부
	bool bLaunched = false;
};
