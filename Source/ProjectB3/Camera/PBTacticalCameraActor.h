// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBTacticalCameraActor.generated.h"

class UCameraComponent;
class UMeshComponent;

/** 전술 카메라 연출 전용 경량 카메라 액터. ViewTarget으로 전환되어 사용된다. */
UCLASS()
class PROJECTB3_API APBTacticalCameraActor : public AActor
{
	GENERATED_BODY()

public:
	APBTacticalCameraActor();

	// 카메라 컴포넌트 반환
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

	// 추적 대상 및 Orbit 파라미터 설정 — 매 틱 대상 위치 기준으로 TargetTransform을 재계산
	void SetTrackingTarget(AActor* InTarget, float InYaw, float InPitch, float InDistance);

	// 추적을 해제하고 고정 Transform으로 보간 이동 (스킬 프레이밍 등)
	void SetTargetTransform(const FTransform& InTargetTransform);

	// 추적 해제 — TargetTransform을 현재 위치로 고정하여 제자리 유지
	void ClearTracking();

	// 추적 대상과 카메라 사이 장애물 컷아웃 갱신
	void UpdateCameraCutout();

protected:
	virtual void Tick(float DeltaTime) override;

public:
	// Transform 보간 속도
	UPROPERTY(EditAnywhere, Category = "TacticalCamera")
	float InterpSpeed = 5.0f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	// 보간 목표 Transform (Tracking 모드에서는 매 틱 갱신됨)
	FTransform TargetTransform;

	// 추적 대상 액터
	TWeakObjectPtr<AActor> TrackingTarget;

	// Orbit 파라미터 (SetTrackingTarget 호출 시 저장)
	float TrackYaw      = 0.0f;
	float TrackPitch    = 0.0f;
	float TrackDistance = 0.0f;

	// 컷아웃 대상 메시 추적 (이전 프레임 히트 목록)
	TSet<TWeakObjectPtr<UMeshComponent>> FadedMeshes;
};
