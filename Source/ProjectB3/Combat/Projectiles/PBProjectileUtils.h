// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Bezier 곡선 계산 유틸리티. APBProjectile과 UPBTargetingComponent에서 공유.
namespace PBProjectileUtils
{
	// Quadratic Bezier 중간 제어점 계산.
	// Start → End 중점 위에 ArcHeight만큼 올린 점을 반환.
	FVector CalcMidControlPoint(
		const FVector& Start, const FVector& End,
		float ArcHeightRatio, float MinArcHeight, float MaxArcHeight);

	// Quadratic Bezier 위치 보간. t ∈ [0, 1].
	FVector BezierPoint(const FVector& P0, const FVector& P1, const FVector& P2, float t);

	// Quadratic Bezier 탄젠트. t ∈ [0, 1].
	FVector BezierTangent(const FVector& P0, const FVector& P1, const FVector& P2, float t);
}
