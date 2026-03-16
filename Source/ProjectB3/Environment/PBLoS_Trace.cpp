// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBLoS_Trace.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Actor.h"

FPBLoSResult UPBLoS_Trace::Execute(
	const UWorld* World,
	const FVector& Source,
	const AActor* Target) const
{
	FPBLoSResult Result;

	if (!IsValid(World) || !IsValid(Target))
	{
		return Result;
	}

	const FVector TargetLocation = Target->GetActorLocation();

	// 고도 차이 산출 (양수 = 시전자가 높음)
	Result.ElevationDelta = Source.Z - TargetLocation.Z;

	// Trace 시작/끝 위치: 캐릭터 중심 높이 보정
	const FVector TraceStart = Source + FVector(0.0f, 0.0f, TraceHeightOffset);
	const FVector TraceEnd = TargetLocation + FVector(0.0f, 0.0f, TraceHeightOffset);

	// Trace 파라미터: 시전자 자신과 대상은 무시
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LoSTrace), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(Target);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// 장애물에 막히지 않았으면 시야 확보
	Result.bHasLineOfSight = !bHit;

	// TODO: 엄폐 수준 판정 (Hit된 오브젝트의 Cover 속성 조회)
	// 현재는 시야 여부만 반환, 추후 Cover 시스템 구현 시 확장
	if (bHit)
	{
		Result.CoverLevel = EPBCoverLevel::Full;
	}

	return Result;
}
