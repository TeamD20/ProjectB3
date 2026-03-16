// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvironmentSubsystem.h"
#include "PBLineOfSightStrategy.h"
#include "PBLoS_Trace.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UPBEnvironmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 기본 전략: Trace 기반 LoS
	// TODO: Project Settings / DataAsset에서 DefaultLoSStrategy 읽기
	LoSStrategy = NewObject<UPBLoS_Trace>(this);
}

void UPBEnvironmentSubsystem::Deinitialize()
{
	EndLoSCache();
	LoSStrategy = nullptr;

	Super::Deinitialize();
}

// 시야 판정

FPBLoSResult UPBEnvironmentSubsystem::CheckLineOfSight(const FVector& Source, const AActor* Target) const
{
	if (!IsValid(LoSStrategy) || !IsValid(Target))
	{
		return FPBLoSResult();
	}

	// 캐시 비활성 → 전략 직접 호출
	if (!bLoSCacheActive)
	{
		return LoSStrategy->Execute(GetWorld(), Source, Target);
	}

	// 캐시 키 생성
	const FPBLoSCacheKey Key(Source, Target, LoSCacheTolerance);

	// 캐시 히트 → 즉시 반환
	if (const FPBLoSResult* CachedResult = LoSCache.Find(Key))
	{
		return *CachedResult;
	}

	// 캐시 미스 → 전략 실행 → 결과 캐싱
	FPBLoSResult Result = LoSStrategy->Execute(GetWorld(), Source, Target);
	LoSCache.Add(Key, Result);
	return Result;
}

TArray<AActor*> UPBEnvironmentSubsystem::GetVisibleTargetsFrom(
	const FVector& Position, float MaxRange, const TArray<AActor*>& CandidateTargets) const
{
	TArray<AActor*> VisibleTargets;

	const float MaxRangeSq = MaxRange * MaxRange;

	for (AActor* Candidate : CandidateTargets)
	{
		if (!IsValid(Candidate))
		{
			continue;
		}

		// 사거리 사전 필터
		if (MaxRange > 0.0f && FVector::DistSquared(Position, Candidate->GetActorLocation()) > MaxRangeSq)
		{
			continue;
		}

		const FPBLoSResult LoSResult = CheckLineOfSight(Position, Candidate);
		if (LoSResult.bHasLineOfSight)
		{
			VisibleTargets.Add(Candidate);
		}
	}

	return VisibleTargets;
}

void UPBEnvironmentSubsystem::SetLoSStrategy(TSubclassOf<UPBLineOfSightStrategy> StrategyClass)
{
	if (!StrategyClass)
	{
		return;
	}

	LoSStrategy = NewObject<UPBLineOfSightStrategy>(this, StrategyClass);

	// 전략 교체 시 기존 캐시 파기
	LoSCache.Empty();
	PathCostCache.Empty();
}

// 경로 거리 산정

float UPBEnvironmentSubsystem::CalculatePathDistance(const TArray<FVector>& PathPoints) const
{
	float TotalDist = 0.0f;
	for (int32 i = 1; i < PathPoints.Num(); ++i)
	{
		TotalDist += FVector::Dist(PathPoints[i - 1], PathPoints[i]);
	}
	return TotalDist;
}

FPBPathCostResult UPBEnvironmentSubsystem::CalculatePathCost(const FVector& Start, const FVector& End) const
{
	FPBPathCostResult Result;

	// 캐시 조회 (Start/End 좌표로 해시)
	const uint64 CacheKey = HashCombine(GetTypeHash(FIntVector(Start)), GetTypeHash(FIntVector(End)));
	if (bLoSCacheActive)
	{
		if (const FPBPathCostResult* CachedResult = PathCostCache.Find(CacheKey))
		{
			return *CachedResult;
		}
	}

	// NavSystem 경로 탐색
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return Result;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!IsValid(NavSys))
	{
		return Result;
	}

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(World, Start, End);
	if (!IsValid(NavPath) || !NavPath->IsValid())
	{
		return Result;
	}

	Result.PathPoints = NavPath->PathPoints;
	Result.TotalCost = CalculatePathDistance(NavPath->PathPoints);
	Result.bIsValid = true;

	// 캐시 저장
	if (bLoSCacheActive)
	{
		PathCostCache.Add(CacheKey, Result);
	}

	return Result;
}

float UPBEnvironmentSubsystem::CalculateDistanceAlongPath(
	const TArray<FVector>& PathPoints, const FVector& CurrentLocation) const
{
	// 기존 PBGameplayAbility_Move::CalculateDistanceAlongPath 로직 이관
	// MovePathPoints의 각 세그먼트에 현재 위치를 투영(projection)하여
	// 가장 가까운 세그먼트를 찾고, 시작점부터 해당 투영 지점까지의 경로 누적 거리를 반환
	if (PathPoints.Num() < 2)
	{
		return 0.0f;
	}

	// 1. 현재 위치에 가장 가까운 경로 세그먼트 탐색
	float NearestDistSq = MAX_FLT;
	int32 NearestSegIndex = 0;
	float NearestSegRatio = 0.0f;

	for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
	{
		const FVector SegStart = PathPoints[i];
		const FVector SegEnd = PathPoints[i + 1];
		const FVector ProjectedPoint = FMath::ClosestPointOnSegment(CurrentLocation, SegStart, SegEnd);
		const float DistSqToProjection = FVector::DistSquared(CurrentLocation, ProjectedPoint);

		if (DistSqToProjection < NearestDistSq)
		{
			NearestDistSq = DistSqToProjection;
			NearestSegIndex = i;

			const float SegLength = FVector::Dist(SegStart, SegEnd);
			NearestSegRatio = (SegLength > 0.0f)
				? FVector::Dist(SegStart, ProjectedPoint) / SegLength
				: 0.0f;
		}
	}

	// 2. 시작점 ~ 투영 지점까지의 경로 누적 거리 계산
	float TotalTraveledDist = 0.0f;
	for (int32 i = 0; i < NearestSegIndex; ++i)
	{
		TotalTraveledDist += FVector::Dist(PathPoints[i], PathPoints[i + 1]);
	}
	// 현재 세그먼트의 부분 거리 추가
	TotalTraveledDist += FVector::Dist(PathPoints[NearestSegIndex], PathPoints[NearestSegIndex + 1]) * NearestSegRatio;

	return TotalTraveledDist;
}

FVector UPBEnvironmentSubsystem::CalculateClampedDestination(
	const TArray<FVector>& PathPoints, float MaxDistance, TArray<FVector>& OutClampedPath) const
{
	// 기존 PBGameplayAbility_Move::CalculateClampedDestination 로직 이관
	if (PathPoints.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	// 무제한 이동력: 전체 경로 반환
	if (MaxDistance <= -1.0f)
	{
		OutClampedPath = PathPoints;
		return PathPoints.Last();
	}

	OutClampedPath.Add(PathPoints[0]);
	float AccumulatedDist = 0.0f;

	for (int32 i = 1; i < PathPoints.Num(); ++i)
	{
		const float SegDist = FVector::Dist(PathPoints[i - 1], PathPoints[i]);

		if (AccumulatedDist + SegDist >= MaxDistance)
		{
			const float Remaining = MaxDistance - AccumulatedDist;
			const float T = (SegDist > 0.0f) ? (Remaining / SegDist) : 0.0f;
			const FVector ClampedPoint = FMath::Lerp(PathPoints[i - 1], PathPoints[i], T);
			OutClampedPath.Add(ClampedPoint);
			return ClampedPoint;
		}

		OutClampedPath.Add(PathPoints[i]);
		AccumulatedDist += SegDist;
	}

	return PathPoints.Last();
}

// 캐시 수명 관리

void UPBEnvironmentSubsystem::BeginLoSCache()
{
	bLoSCacheActive = true;
	LoSCache.Empty();
	PathCostCache.Empty();
}

void UPBEnvironmentSubsystem::EndLoSCache()
{
	bLoSCacheActive = false;
	LoSCache.Empty();
	PathCostCache.Empty();
}
