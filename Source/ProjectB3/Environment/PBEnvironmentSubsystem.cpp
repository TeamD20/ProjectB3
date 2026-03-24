// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEnvironmentSubsystem.h"
#include "PBLineOfSightStrategy.h"
#include "PBLoS_Trace.h"
#include "ProjectB3/Combat/DamageArea/PBDamageArea.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
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
	EndEnvironmentCache();
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
	if (!bEnvironmentCacheActive)
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

FPBPathFindResult UPBEnvironmentSubsystem::CalculatePath(const FVector& Start, const FVector& End) const
{
	FPBPathFindResult Result;

	// 캐시 조회 (25cm 격자 양자화 키)
	auto QuantizeVec = [](const FVector& V) -> FIntVector
	{
		return FIntVector(
			FMath::RoundToInt(V.X * 0.04f),
			FMath::RoundToInt(V.Y * 0.04f),
			FMath::RoundToInt(V.Z * 0.04f)
		);
	};
	const uint64 CacheKey = HashCombine(GetTypeHash(QuantizeVec(Start)), GetTypeHash(QuantizeVec(End)));
	if (bEnvironmentCacheActive)
	{
		if (const FPBPathFindResult* CachedResult = PathCostCache.Find(CacheKey))
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
	if (bEnvironmentCacheActive)
	{
		PathCostCache.Add(CacheKey, Result);
	}

	return Result;
}

FPBPathFindResult UPBEnvironmentSubsystem::CalculatePathForAgent(const AController* Controller, const FVector& GoalLocation, bool bAllowPartialPath) const
{
	FPBPathFindResult Result;
	if (!IsValid(Controller) || !IsValid(Controller->GetPawn()))
	{
		return Result;
	}

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

	const FVector AgentNavLocation = Controller->GetNavAgentLocation();
	const ANavigationData* NavData = NavSys->GetNavDataForProps(Controller->GetNavAgentPropertiesRef(), AgentNavLocation);
	if (!IsValid(NavData))
	{
		return Result;
	}

	// 25cm 격자 양자화 키 — ImpactPoint와 Location의 미세한 차이 및 PathUpdateMinDistance 내 커서 드리프트 흡수
	auto QuantizeVec = [](const FVector& V) -> FIntVector
	{
		return FIntVector(
			FMath::RoundToInt(V.X * 0.04f),
			FMath::RoundToInt(V.Y * 0.04f),
			FMath::RoundToInt(V.Z * 0.04f)
		);
	};
	const uint64 CacheKey = HashCombine(GetTypeHash(QuantizeVec(AgentNavLocation)), GetTypeHash(QuantizeVec(GoalLocation)));

	// 캐시 활성 시 조회 먼저
	if (bEnvironmentCacheActive)
	{
		if (const FPBPathFindResult* Cached = PathCostCache.Find(CacheKey))
		{
			return *Cached;
		}
	}

	FPathFindingQuery Query(Controller, *NavData, AgentNavLocation, GoalLocation);
	Query.SetAllowPartialPaths(bAllowPartialPath);

	const FPathFindingResult PathResult = NavSys->FindPathSync(Query);
	if (!PathResult.IsSuccessful() || !PathResult.Path.IsValid())
	{
		return Result;
	}

	const TArray<FNavPathPoint>& NavPathPoints = PathResult.Path->GetPathPoints();
	Result.PathPoints.Reserve(NavPathPoints.Num());
	for (const FNavPathPoint& NavPathPoint : NavPathPoints)
	{
		Result.PathPoints.Add(NavPathPoint.Location);
	}

	Result.TotalCost = CalculatePathDistance(Result.PathPoints);
	Result.bIsValid = true;

	// 캐시 활성 시 결과 저장 — NavPath도 함께 보관하여 RequestMoveToLocation에서 재사용
	if (bEnvironmentCacheActive)
	{
		PathCostCache.Add(CacheKey, Result);
		NavPathCache.Add(CacheKey, PathResult.Path);
	}

	return Result;
}

bool UPBEnvironmentSubsystem::RequestMoveToLocation(AController* Controller, const FVector& GoalLocation, float AcceptanceRadius, bool bAllowPartialPath) const
{
	if (!IsValid(Controller) || !IsValid(Controller->GetPawn()))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return false;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
	if (!IsValid(NavSys))
	{
		return false;
	}

	UPathFollowingComponent* PathFollowingComp = nullptr;
	if (AAIController* AIController = Cast<AAIController>(Controller))
	{
		PathFollowingComp = AIController->GetPathFollowingComponent();
	}
	else
	{
		PathFollowingComp = Controller->FindComponentByClass<UPathFollowingComponent>();
	}

	if (!IsValid(PathFollowingComp) || !PathFollowingComp->IsPathFollowingAllowed())
	{
		return false;
	}

	const bool bAlreadyAtGoal = PathFollowingComp->HasReached(GoalLocation, EPathFollowingReachMode::OverlapAgent);
	if (PathFollowingComp->GetStatus() != EPathFollowingStatus::Idle)
	{
		PathFollowingComp->AbortMove(*NavSys,
			FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest,
			FAIRequestID::AnyRequest,
			bAlreadyAtGoal ? EPathFollowingVelocityMode::Reset : EPathFollowingVelocityMode::Keep);
	}

	if (bAlreadyAtGoal)
	{
		PathFollowingComp->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
		return true;
	}

	const FVector AgentNavLocation = Controller->GetNavAgentLocation();
	const ANavigationData* NavData = NavSys->GetNavDataForProps(Controller->GetNavAgentPropertiesRef(), AgentNavLocation);
	if (!IsValid(NavData))
	{
		return false;
	}

	// 캐시 히트 시 FindPathSync 재호출 없이 저장된 NavPath 재사용 (25cm 격자 양자화)
	auto QuantizeVec = [](const FVector& V) -> FIntVector
	{
		return FIntVector(
			FMath::RoundToInt(V.X * 0.04f),
			FMath::RoundToInt(V.Y * 0.04f),
			FMath::RoundToInt(V.Z * 0.04f)
		);
	};
	const uint64 CacheKey = HashCombine(GetTypeHash(QuantizeVec(AgentNavLocation)), GetTypeHash(QuantizeVec(GoalLocation)));

	FNavPathSharedPtr PathToUse;
	if (bEnvironmentCacheActive)
	{
		if (const FNavPathSharedPtr* CachedNavPath = NavPathCache.Find(CacheKey))
		{
			if (CachedNavPath->IsValid())
			{
				PathToUse = *CachedNavPath;
			}
		}
	}

	if (!PathToUse.IsValid())
	{
		FPathFindingQuery Query(Controller, *NavData, AgentNavLocation, GoalLocation);
		Query.SetAllowPartialPaths(bAllowPartialPath);
		const FPathFindingResult PathResult = NavSys->FindPathSync(Query);
		if (!PathResult.IsSuccessful() || !PathResult.Path.IsValid())
		{
			if (PathFollowingComp->GetStatus() != EPathFollowingStatus::Idle)
			{
				PathFollowingComp->RequestMoveWithImmediateFinish(EPathFollowingResult::Invalid);
			}
			return false;
		}
		PathToUse = PathResult.Path;
	}

	Controller->GetPawn()->SetCanAffectNavigationGeneration(false);

	FAIMoveRequest MoveRequest(GoalLocation);
	MoveRequest.SetUsePathfinding(true);
	MoveRequest.SetAllowPartialPath(bAllowPartialPath);
	MoveRequest.SetAcceptanceRadius(AcceptanceRadius);

	PathFollowingComp->RequestMove(MoveRequest, PathToUse);
	return true;
}

float UPBEnvironmentSubsystem::CalculateDistanceAlongPath(
	const TArray<FVector>& PathPoints, const FVector& CurrentLocation) const
{
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

// 위험 영역 판정

void UPBEnvironmentSubsystem::RegisterDamageArea(APBDamageArea* Area)
{
	if (IsValid(Area))
	{
		RegisteredDamageAreas.AddUnique(Area);
	}
}

void UPBEnvironmentSubsystem::UnregisterDamageArea(APBDamageArea* Area)
{
	RegisteredDamageAreas.Remove(Area);
}

FPBHazardQueryResult UPBEnvironmentSubsystem::QueryHazardAtPoint(const FVector& Point) const
{
	// 캐시 키 생성 (LoS와 동일한 양자화 기준)
	const float InvTolerance = (LoSCacheTolerance > 0.0f) ? (1.0f / LoSCacheTolerance) : 1.0f;
	const FIntVector CacheKey(
		FMath::RoundToInt(Point.X * InvTolerance),
		FMath::RoundToInt(Point.Y * InvTolerance),
		FMath::RoundToInt(Point.Z * InvTolerance)
	);

	// 캐시 히트 → 즉시 반환
	if (bEnvironmentCacheActive)
	{
		if (const FPBHazardQueryResult* CachedResult = HazardCache.Find(CacheKey))
		{
			return *CachedResult;
		}
	}

	// 등록된 DamageArea 순회하여 구체 포함 판정
	FPBHazardQueryResult Result;
	for (const TWeakObjectPtr<APBDamageArea>& WeakArea : RegisteredDamageAreas)
	{
		APBDamageArea* Area = WeakArea.Get();
		if (!IsValid(Area))
		{
			continue;
		}

		const float Radius = Area->GetAreaRadius();
		const float DistSq = FVector::DistSquared(Point, Area->GetActorLocation());
		if (DistSq <= Radius * Radius)
		{
			Result.bIsInHazard = true;
			Result.TotalExpectedDamage += Area->GetExpectedDamage();
			Result.OverlappingAreas.Add(Area);
		}
	}

	// 캐시 저장
	if (bEnvironmentCacheActive)
	{
		HazardCache.Add(CacheKey, Result);
	}

	return Result;
}

// 영역 효과 중복 적용 방지

bool UPBEnvironmentSubsystem::TryMarkAreaEffectApplied(
	const AActor* Target, const UGameplayEffect* EffectDef, int32 CurrentRound)
{
	// 라운드가 바뀌면 기존 기록 초기화
	if (CurrentRound != AreaEffectDedupRound)
	{
		AreaEffectDedupRound = CurrentRound;
		AreaEffectDedupSet.Empty();
	}

	const uint64 Key = HashCombine(GetTypeHash(Target), GetTypeHash(EffectDef));
	if (AreaEffectDedupSet.Contains(Key))
	{
		return false;
	}
	AreaEffectDedupSet.Add(Key);
	return true;
}

// 캐시 수명 관리

void UPBEnvironmentSubsystem::BeginEnvironmentCache()
{
	bEnvironmentCacheActive = true;
	LoSCache.Empty();
	PathCostCache.Empty();
	NavPathCache.Empty();
	HazardCache.Empty();
}

void UPBEnvironmentSubsystem::EndEnvironmentCache()
{
	bEnvironmentCacheActive = false;
	LoSCache.Empty();
	PathCostCache.Empty();
	NavPathCache.Empty();
	HazardCache.Empty();
}
