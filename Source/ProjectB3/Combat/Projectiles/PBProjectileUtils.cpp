// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBProjectileUtils.h"

FVector PBProjectileUtils::CalcMidControlPoint(
	const FVector& Start, const FVector& End,
	float ArcHeightRatio, float MinArcHeight, float MaxArcHeight)
{
	const FVector MidPoint = (Start + End) * 0.5f;
	const float HorizontalDist = FVector::Dist2D(Start, End);
	const float ArcHeight = FMath::Clamp(HorizontalDist * ArcHeightRatio, MinArcHeight, MaxArcHeight);

	return FVector(MidPoint.X, MidPoint.Y, FMath::Max(Start.Z, End.Z) + ArcHeight);
}

FVector PBProjectileUtils::BezierPoint(const FVector& P0, const FVector& P1, const FVector& P2, float t)
{
	const float u = 1.f - t;
	return u * u * P0 + 2.f * u * t * P1 + t * t * P2;
}

FVector PBProjectileUtils::BezierTangent(const FVector& P0, const FVector& P1, const FVector& P2, float t)
{
	return 2.f * (1.f - t) * (P1 - P0) + 2.f * t * (P2 - P1);
}
