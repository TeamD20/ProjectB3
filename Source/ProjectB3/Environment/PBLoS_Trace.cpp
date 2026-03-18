// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBLoS_Trace.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

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

	// D&D 5e 규칙: 캐릭터(Pawn)는 LoS를 차단하지 않음 (커버만 제공)
	// Pawn에 맞으면 무시하고 재트레이스 (최대 10회 반복 — 무한 루프 방지)
	FHitResult HitResult;
	bool bHit = false;
	static constexpr int32 MaxPawnSkips = 10;

	for (int32 Attempt = 0; Attempt <= MaxPawnSkips; ++Attempt)
	{
		bHit = World->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			QueryParams
		);

		if (!bHit)
		{
			break; // 장애물 없음 → 시야 확보
		}

		const AActor* HitActor = HitResult.GetActor();
		if (IsValid(HitActor) && HitActor->IsA<APawn>())
		{
			// Pawn에 맞음 → 무시 목록에 추가하고 재트레이스
			QueryParams.AddIgnoredActor(HitActor);
			continue;
		}

		break; // 벽/지형에 맞음 → 차단 확정
	}

	// 장애물에 막히지 않았으면 시야 확보
	Result.bHasLineOfSight = !bHit;

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
