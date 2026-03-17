// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBLoS_Trace.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Actor.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBLoS, Log, All);

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

	// Trace 파라미터: 대상은 무시 (자기 자신의 캡슐 관통 방지)
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

	// 디버그 시각화: 녹색 = 시야 확보, 빨간색 = 차단
#if ENABLE_DRAW_DEBUG
	if (const UWorld* MutableWorld = const_cast<UWorld*>(World))
	{
		const FColor LineColor = Result.bHasLineOfSight ? FColor::Green : FColor::Red;
		DrawDebugLine(MutableWorld, TraceStart, TraceEnd, LineColor,
			/*bPersistentLines=*/false, /*LifeTime=*/3.0f, /*DepthPriority=*/0, /*Thickness=*/2.0f);

		// 차단된 경우: 히트 지점에 구체 표시
		if (bHit)
		{
			DrawDebugSphere(MutableWorld, HitResult.ImpactPoint, 15.0f, 8,
				FColor::Red, false, 3.0f);
		}
	}
#endif

	// 로그: 차단 시 장애물 액터 이름 출력
	if (bHit)
	{
		const AActor* BlockingActor = HitResult.GetActor();
		UE_LOG(LogPBLoS, Display,
			TEXT("[LoS] 차단됨: Source(%s) → Target(%s) | "
			     "BlockedBy=[%s] at (%s) | TraceZ: %.0f→%.0f"),
			*Source.ToCompactString(),
			*Target->GetName(),
			IsValid(BlockingActor) ? *BlockingActor->GetName() : TEXT("None"),
			*HitResult.ImpactPoint.ToCompactString(),
			TraceStart.Z, TraceEnd.Z);

		Result.CoverLevel = EPBCoverLevel::Full;
	}
	else
	{
		UE_LOG(LogPBLoS, Verbose,
			TEXT("[LoS] 확보됨: Source(%s) → Target(%s) | TraceZ: %.0f→%.0f"),
			*Source.ToCompactString(),
			*Target->GetName(),
			TraceStart.Z, TraceEnd.Z);
	}

	return Result;
}
