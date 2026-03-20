// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPathDisplayComponent.h"
#include "Components/SplineMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "ProjectB3/Utils/PBDebugUtils.h"

UPBPathDisplayComponent::UPBPathDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPBPathDisplayComponent::BeginPlay()
{
	Super::BeginPlay();

	ValidateProperties();

	// 경로 시각화 전용 액터 소환 (월드 원점에 배치, SplineMeshComponent 풀 소유)
	VisualActor = GetWorld()->SpawnActor<AActor>();
}

void UPBPathDisplayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// VisualActor는 월드에 독립적으로 소환되었으므로 명시적으로 파괴
	if (IsValid(VisualActor))
	{
		VisualActor->Destroy();
		VisualActor = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UPBPathDisplayComponent::SetPathDisplayEnabled(bool bEnabled)
{
	bPathDisplayEnabled = bEnabled;
	if (!bEnabled)
	{
		ClearPath();
	}
}

void UPBPathDisplayComponent::DisplayPath(const TArray<FVector>& PathPoints, bool bDisplayDistance)
{
	if (!bPathDisplayEnabled)
	{
		return;
	}
	if (PathPoints.Num() < 2)
	{
		ClearPath();
		return;
	}

	// 표시용 시작부 포인트 정제 조건 (실제 이동 경로에는 영향 없음)
	// - 대상 포인트: [0,1,2] 중 1번 포인트만 검사/제거 (0번은 그대로 유지)
	// - 길이 게이트: Dist2D(P0, P1) <= 220cm 인 경우에만 제거 후보로 본다.
	// - 제거 조건 A (직선 근접):
	//   P1을 선분(P0->P2)에 투영했을 때 횡오프셋 Dist2D(P1, Projected) <= 45cm
	// - 제거 조건 B (전진 기여 부족):
	//   Dist2D(P1, P2) >= Dist2D(P0, P2) - 5cm
	// - 제거 조건 C (진행 방향 역행):
	//   Dot2D(Normal(P0->P1), Normal(P0->P2)) <= 0.0
	// - 제거 조건 D (시작부 급회전):
	//   Dot2D(Normal(P0->P1), Normal(P1->P2)) <= 0.35
	// - A/B/C/D 중 하나라도 만족하면 1번 포인트 제거
	// - 최대 2회 반복하여 시작부의 1~2번 잡음 포인트만 정리
	TArray<FVector> DisplayPathPoints = PathPoints;
	auto ShouldPruneEarlyPoint = [](const FVector& P0, const FVector& P1, const FVector& P2) -> bool
	{
		const float SegmentLength2D = FVector::Dist2D(P0, P1);
		if (SegmentLength2D > 220.0f)
		{
			return false;
		}

		const FVector Projected = FMath::ClosestPointOnSegment(P1, P0, P2);
		const float LateralOffset2D = FVector::Dist2D(P1, Projected);
		const bool bNearStraightLine = LateralOffset2D <= 45.0f;

		const float DistToGoalFromP0 = FVector::Dist2D(P0, P2);
		const float DistToGoalFromP1 = FVector::Dist2D(P1, P2);
		const bool bNoForwardProgress = DistToGoalFromP1 >= DistToGoalFromP0 - 5.0f;

		const FVector Dir01 = FVector(P1.X - P0.X, P1.Y - P0.Y, 0.f).GetSafeNormal();
		const FVector Dir02 = FVector(P2.X - P0.X, P2.Y - P0.Y, 0.f).GetSafeNormal();
		const FVector Dir12 = FVector(P2.X - P1.X, P2.Y - P1.Y, 0.f).GetSafeNormal();

		const bool bBackwardsFromStart = !Dir01.IsNearlyZero() && !Dir02.IsNearlyZero() && FVector::DotProduct(Dir01, Dir02) <= 0.0f;
		const bool bSharpTurnAtP1 = !Dir01.IsNearlyZero() && !Dir12.IsNearlyZero() && FVector::DotProduct(Dir01, Dir12) <= 0.35f;

		return bNearStraightLine || bNoForwardProgress || bBackwardsFromStart || bSharpTurnAtP1;
	};

	int32 PruneCount = 0;
	// 시작부만 정제하기 위해 최대 2회까지만 반복
	while (DisplayPathPoints.Num() >= 3 && PruneCount < 2 &&
		ShouldPruneEarlyPoint(DisplayPathPoints[0], DisplayPathPoints[1], DisplayPathPoints[2]))
	{
		DisplayPathPoints.RemoveAt(1);
		PruneCount++;
	}

	FPBPathDrawData DrawData;
	DrawData.BasePathPoints = DisplayPathPoints;
	BuildTerrainSnappedPoints(DisplayPathPoints, DrawData);
	BuildCurvedInterpolationPoints(DrawData);
	CalculateTotalDistance(DrawData);
	CalculateSplitDistance(DrawData);
	//DrawDebugPath(DrawData);
	RebuildLineSegments(DrawData);

	if (bDisplayDistance)
	{
		DisplayDistance(DrawData);
	}
}

void UPBPathDisplayComponent::ClearPath()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FlushPersistentDebugLines(World);
	HideAllSegments();
}

void UPBPathDisplayComponent::BeginPathTracking(const TArray<FVector>& PathPoints)
{
	TrackedPathPoints = PathPoints;
}

void UPBPathDisplayComponent::UpdateTrackedPath(const FVector& CurrentLocation)
{
	if (TrackedPathPoints.Num() < 2)
	{
		return;
	}

	const TArray<FVector> RemainingPath = ExtractRemainingPath(CurrentLocation);
	if (RemainingPath.Num() < 2)
	{
		ClearPath();
		return;
	}

	// 이동 중엔 거리 텍스트 불필요
	DisplayPath(RemainingPath, false);
}

void UPBPathDisplayComponent::EndPathTracking()
{
	TrackedPathPoints.Reset();
	ClearPath();
}

TArray<FVector> UPBPathDisplayComponent::ExtractRemainingPath(const FVector& CurrentLocation) const
{
	// 가장 가까운 세그먼트 탐색
	float NearestDistSq = MAX_FLT;
	int32 NearestSegIndex = 0;

	for (int32 i = 0; i < TrackedPathPoints.Num() - 1; ++i)
	{
		const FVector Projected = FMath::ClosestPointOnSegment(CurrentLocation, TrackedPathPoints[i], TrackedPathPoints[i + 1]);
		const float DistSq = FVector::DistSquared(CurrentLocation, Projected);
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestSegIndex = i;
		}
	}

	// 투영점을 첫 포인트로 하여 잔여 경로 구성
	const FVector ProjectedPoint = FMath::ClosestPointOnSegment(
		CurrentLocation, TrackedPathPoints[NearestSegIndex], TrackedPathPoints[NearestSegIndex + 1]);

	TArray<FVector> Remaining;
	Remaining.Add(ProjectedPoint);
	for (int32 i = NearestSegIndex + 1; i < TrackedPathPoints.Num(); ++i)
	{
		Remaining.Add(TrackedPathPoints[i]);
	}
	return Remaining;
}

void UPBPathDisplayComponent::ValidateProperties()
{
	// 필수 에셋 프로퍼티 검증
	if (!IsValid(LineMesh))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PBPathDisplayComponent] LineMesh가 설정되지 않았습니다. (%s)"), *GetOwner()->GetName());
	}
	if (!IsValid(InRangeMaterial))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PBPathDisplayComponent] InRangeMaterial이 설정되지 않았습니다. (%s)"), *GetOwner()->GetName());
	}
	if (!IsValid(OutOfRangeMaterial))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PBPathDisplayComponent] OutOfRangeMaterial이 설정되지 않았습니다. (%s)"), *GetOwner()->GetName());
	}

	// 수치 프로퍼티 유효성 검증
	if (MaxSegmentPoolSize <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[PBPathDisplayComponent] MaxSegmentPoolSize가 0 이하입니다. (%d) (%s)"), MaxSegmentPoolSize, *GetOwner()->GetName());
	}
}


void UPBPathDisplayComponent::BuildTerrainSnappedPoints(const TArray<FVector>& NavPathPoints,
	FPBPathDrawData& OutDrawData) const
{
	const FVector BaseOffsetVec(0.f, 0.f, BasePathZOffset);
	const FVector CorrectionOffsetVec(0.f,0.f,CorrectionPathZOffset);
	
	UWorld* World = GetWorld();
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	OutDrawData.PathPoints.Reset();

	// 0번(시작) 포인트 지면 보정 (캐릭터가 경사면에 있을 때 경로의 첫 번째 점이 지면 아래로 떨어지는 현상 방지)
	FVector StartPoint = NavPathPoints[0];
	bool bSnapepd = false;

	if (IsValid(World) && NavPathPoints.Num() > 1)
	{
		const FVector& NextPoint = NavPathPoints[1];
		const float HeightDiff = FMath::Abs(NextPoint.Z - StartPoint.Z);

		// 다음 포인트와의 높이 차이가 TerrainSnapHeightThreshold 이상일 경우 보정
		if (HeightDiff > TerrainSnapHeightThreshold)
		{
			const float StartZ = FMath::Max(StartPoint.Z, NextPoint.Z) + TerrainTraceStartOffset;
			const float EndZ = FMath::Min(StartPoint.Z, NextPoint.Z) - TerrainTraceEndOffset;
			
			const FVector TraceStart = FVector(StartPoint.X, StartPoint.Y, StartZ);
			const FVector TraceEnd   = FVector(StartPoint.X, StartPoint.Y, EndZ);

			FHitResult HitResult;
			// TODO: Visibility 대신 Ground 채널 활용
			if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
			{
				StartPoint = HitResult.ImpactPoint + CorrectionOffsetVec;
				bSnapepd = true;
			}
		}
	}

	if (!bSnapepd)
	{
		StartPoint += BaseOffsetVec;
	}

	OutDrawData.PathPoints.Add(StartPoint);

	for (int32 i = 1; i < NavPathPoints.Num(); ++i)
	{
		const FVector& SegStart = NavPathPoints[i - 1];
		const FVector& SegEnd   = NavPathPoints[i];
		const float SegDist2D   = FVector::Dist2D(SegStart, SegEnd);
		const float HeightDiff  = FMath::Abs(SegEnd.Z - SegStart.Z);

		if (IsValid(World) && SegDist2D > TerrainSnapLengthThreshold && HeightDiff > TerrainSnapHeightThreshold)
		{
			const int32 SubdivCount = FMath::FloorToInt(SegDist2D / TerrainSnapLengthThreshold);
			const float StartZ      = FMath::Max(SegStart.Z, SegEnd.Z) + TerrainTraceStartOffset;
			const float EndZ        = FMath::Min(SegStart.Z, SegEnd.Z) - TerrainTraceEndOffset;

			for (int32 j = 1; j < SubdivCount; ++j)
			{
				const float T = static_cast<float>(j) / static_cast<float>(SubdivCount);
				const FVector InterpPos  = FMath::Lerp(SegStart, SegEnd, T);
				const FVector TraceStart = FVector(InterpPos.X, InterpPos.Y, StartZ);
				const FVector TraceEnd   = FVector(InterpPos.X, InterpPos.Y, EndZ);

				FHitResult HitResult;
				// TODO: Visibility 대신 Ground 채널 활용
				if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
				{
					const FVector SnappedPoint = HitResult.ImpactPoint + CorrectionOffsetVec;
					OutDrawData.PathPoints.Add(SnappedPoint);
					OutDrawData.CorrectionPoints.Add(SnappedPoint);
				}
			}
		}

		OutDrawData.PathPoints.Add(SegEnd + BaseOffsetVec);
	}
}

void UPBPathDisplayComponent::BuildCurvedInterpolationPoints(FPBPathDrawData& InOutDrawData) const
{
	if (InOutDrawData.PathPoints.Num() < 3 || CornerSmoothingSubdivisions <= 0)
	{
		return;
	}

	// 베지어 곡선 보간
	const TArray<FVector> SourcePoints = InOutDrawData.PathPoints;
	TArray<FVector> SmoothedPoints;
	SmoothedPoints.Reserve(SourcePoints.Num() * 2);
	SmoothedPoints.Add(SourcePoints[0]);

	for (int32 i = 1; i < SourcePoints.Num() - 1; ++i)
	{
		const FVector& Prev = SourcePoints[i - 1];
		const FVector& Curr = SourcePoints[i];
		const FVector& Next = SourcePoints[i + 1];

		const float LenIn2D = FVector::Dist2D(Prev, Curr);
		const float LenOut2D = FVector::Dist2D(Curr, Next);
		if (LenIn2D < CornerSmoothingMinSegmentLength || LenOut2D < CornerSmoothingMinSegmentLength)
		{
			SmoothedPoints.Add(Curr);
			continue;
		}

		const FVector DirIn = FVector(Curr.X - Prev.X, Curr.Y - Prev.Y, 0.f).GetSafeNormal();
		const FVector DirOut = FVector(Next.X - Curr.X, Next.Y - Curr.Y, 0.f).GetSafeNormal();
		if (DirIn.IsNearlyZero() || DirOut.IsNearlyZero())
		{
			SmoothedPoints.Add(Curr);
			continue;
		}

		const float Dot = FMath::Clamp(FVector::DotProduct(DirIn, DirOut), -1.0f, 1.0f);
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
		if (AngleDeg < CornerSmoothingMinAngleDeg || AngleDeg > 160.0f)
		{
			SmoothedPoints.Add(Curr);
			continue;
		}

		const float CornerRadius = FMath::Min3(LenIn2D * CornerSmoothingRadiusRatio, LenOut2D * CornerSmoothingRadiusRatio, CornerSmoothingMaxRadius);
		if (CornerRadius <= KINDA_SMALL_NUMBER)
		{
			SmoothedPoints.Add(Curr);
			continue;
		}

		const float TIn = FMath::Clamp(CornerRadius / LenIn2D, 0.0f, 0.49f);
		const float TOut = FMath::Clamp(CornerRadius / LenOut2D, 0.0f, 0.49f);
		const FVector Entry = FMath::Lerp(Curr, Prev, TIn);
		const FVector Exit = FMath::Lerp(Curr, Next, TOut);

		if (FVector::DistSquared(SmoothedPoints.Last(), Entry) > 1.0f)
		{
			SmoothedPoints.Add(Entry);
			InOutDrawData.CorrectionPoints.Add(Entry);
		}

		for (int32 s = 1; s <= CornerSmoothingSubdivisions; ++s)
		{
			const float T = static_cast<float>(s) / static_cast<float>(CornerSmoothingSubdivisions + 1);
			const float OneMinusT = 1.0f - T;
			const FVector BezierPoint =
				(OneMinusT * OneMinusT) * Entry +
				(2.0f * OneMinusT * T) * Curr +
				(T * T) * Exit;
			SmoothedPoints.Add(BezierPoint);
			InOutDrawData.CorrectionPoints.Add(BezierPoint);
		}

		SmoothedPoints.Add(Exit);
		InOutDrawData.CorrectionPoints.Add(Exit);
	}

	SmoothedPoints.Add(SourcePoints.Last());
	InOutDrawData.PathPoints = MoveTemp(SmoothedPoints);
}

void UPBPathDisplayComponent::CalculateTotalDistance(FPBPathDrawData& InOutDrawData) const
{
	// 색상 분할/표시와 동일 기준을 유지하기 위해 보정 포인트를 포함한 PathPoints 기준으로 계산
	InOutDrawData.TotalDistance = 0.0f;
	for (int32 i = 1; i < InOutDrawData.PathPoints.Num(); ++i)
	{
		const float SegmentDist = FVector::Dist(InOutDrawData.PathPoints[i - 1], InOutDrawData.PathPoints[i]);
		InOutDrawData.TotalDistance += SegmentDist;
	}
}

void UPBPathDisplayComponent::CalculateSplitDistance(FPBPathDrawData& InOutDrawData) const
{
	// 무제한 이동: 전체 경로가 InRange
	if (MaxMoveDistance < 0.0f)
	{
		InOutDrawData.SplitDistance = InOutDrawData.TotalDistance;
		return;
	}

	float AccumulatedDist = 0.0f;
	for (int32 i = 1; i < InOutDrawData.PathPoints.Num(); ++i)
	{
		const float SegmentDist = FVector::Dist(InOutDrawData.PathPoints[i - 1], InOutDrawData.PathPoints[i]);
		if (AccumulatedDist + SegmentDist >= MaxMoveDistance)
		{
			InOutDrawData.SplitDistance = MaxMoveDistance;
			return;
		}
		AccumulatedDist += SegmentDist;
	}

	// 전체 경로가 MaxMoveDistance보다 짧으면 전체가 InRange
	InOutDrawData.SplitDistance = AccumulatedDist;
}

void UPBPathDisplayComponent::HideAllSegments()
{
	for (TObjectPtr<USplineMeshComponent>& Segment : SegmentPool)
	{
		if (IsValid(Segment))
		{
			Segment->SetVisibility(false);
		}
	}
}

void UPBPathDisplayComponent::DisplayDistance(const FPBPathDrawData& InDrawData) const
{
	// TODO: UI에 거리 표시
	float MeterDistance = InDrawData.TotalDistance / 100.f;
	FString DistanceText = FString::Printf(TEXT("%.1fm"), MeterDistance);
	DebugUtils::PrintOnScreen(DistanceText,1);
}


void UPBPathDisplayComponent::DrawDebugPath(const FPBPathDrawData& InDrawData) const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	FlushPersistentDebugLines(World);

	const FColor InRangeColor = FColor::Cyan;
	const FColor OutOfRangeColor = FColor::Red;
	constexpr float LineThickness = 5.0f;

	// float AccumulatedDist = 0.0f;
	// for (int32 i = 1; i < InDrawData.PathPoints.Num(); ++i)
	// {
	// 	const FVector& SegStart = InDrawData.PathPoints[i - 1];
	// 	const FVector& SegEnd   = InDrawData.PathPoints[i];
	// 	const float SegDist     = FVector::Dist(SegStart, SegEnd);
	//
	// 	if (AccumulatedDist >= InDrawData.SplitDistance)
	// 	{
	// 		// 세그먼트 전체가 이동 범위 밖
	// 		DrawDebugLine(World, SegStart, SegEnd, OutOfRangeColor, true, -1.0f, 0, LineThickness);
	// 	}
	// 	else if (AccumulatedDist + SegDist <= InDrawData.SplitDistance)
	// 	{
	// 		// 세그먼트 전체가 이동 범위 안
	// 		DrawDebugLine(World, SegStart, SegEnd, InRangeColor, true, -1.0f, 0, LineThickness);
	// 	}
	// 	else
	// 	{
	// 		// 세그먼트 중간에 분기점이 위치할 경우 두 조각으로 분할
	// 		const float RemainingInRange = InDrawData.SplitDistance - AccumulatedDist;
	// 		const float T = (SegDist > 0.0f) ? (RemainingInRange / SegDist) : 0.0f;
	// 		const FVector SplitPoint = FMath::Lerp(SegStart, SegEnd, T);
	//
	// 		DrawDebugLine(World, SegStart, SplitPoint, InRangeColor,    true, -1.0f, 0, LineThickness);
	// 		DrawDebugLine(World, SplitPoint, SegEnd,   OutOfRangeColor, true, -1.0f, 0, LineThickness);
	// 	}
	//
	// 	AccumulatedDist += SegDist;
	// }

	// 기본 포인트 시각화: 파랑
	for (const FVector& Point : InDrawData.BasePathPoints)
	{
		DrawDebugSphere(World, Point + FVector(0.f, 0.f, BasePathZOffset), 7.f, 8, FColor::Blue, true, -1.f);
	}
	// 보정 포인트 시각화: 초록
	for (const FVector& Point : InDrawData.CorrectionPoints)
	{
		DrawDebugSphere(World, Point, 5.f, 6, FColor::Green, true, -1.f);
	}
}

void UPBPathDisplayComponent::RebuildLineSegments(const FPBPathDrawData& DrawData)
{
	if (!IsValid(LineMesh) || !IsValid(VisualActor))
	{
		HideAllSegments();
		return;
	}

	const TArray<FVector>& Points = DrawData.PathPoints;
	const int32 N = Points.Num();

	// Catmull-Rom 탄젠트 계산: 인접 포인트 방향 평균으로 부드러운 곡선 생성
	TArray<FVector> Tangents;
	Tangents.SetNum(N);
	for (int32 i = 0; i < N; ++i)
	{
		if (i == 0)
		{
			Tangents[i] = Points[1] - Points[0];
		}
		else if (i == N - 1)
		{
			Tangents[i] = Points[N - 1] - Points[N - 2];
		}
		else
		{
			Tangents[i] = 0.5f * (Points[i + 1] - Points[i - 1]);
		}
	}

	int32 NextIndex = 0;
	float AccumDist = 0.f;

	auto SetupSegment = [&](USplineMeshComponent* Seg,
		const FVector& WStart, const FVector& WStartTangent,
		const FVector& WEnd,   const FVector& WEndTangent,
		bool bInRange)
	{
		// 현재 세그먼트의 실제 거리 계산
		const float SegmentDistance = FVector::Dist(WStart, WEnd);
		// 코너 곡률 확보를 위해 원본 탄젠트 크기를 보존해 스케일하고,
		// 과도한 오버슈트만 클램프로 제한한다.
		const float TangentScale = TangentTension * 2.0f;
		const float MaxTangentSize = SegmentDistance * 2.5f;
		FVector SafeStartTangent = WStartTangent;
		if (!SafeStartTangent.IsNearlyZero())
		{
			SafeStartTangent = (SafeStartTangent * TangentScale).GetClampedToMaxSize(MaxTangentSize);
		}
		FVector SafeEndTangent = WEndTangent;
		if (!SafeEndTangent.IsNearlyZero())
		{
			SafeEndTangent = (SafeEndTangent * TangentScale).GetClampedToMaxSize(MaxTangentSize);
		}

		Seg->SetStaticMesh(LineMesh);
		Seg->SetMaterial(0, bInRange ? InRangeMaterial : OutOfRangeMaterial);
		Seg->SetStartAndEnd(WStart, SafeStartTangent, WEnd, SafeEndTangent);
		Seg->SetVisibility(true);
	};

	for (int32 i = 1; i < N; ++i)
	{
		const FVector& SegStart = Points[i - 1];
		const FVector& SegEnd   = Points[i];
		const float SegDist     = FVector::Dist(SegStart, SegEnd);

		if (AccumDist >= DrawData.SplitDistance)
		{
			// 세그먼트 전체가 이동 범위 밖
			if (USplineMeshComponent* Seg = GetOrCreateSegment(NextIndex++))
			{
				SetupSegment(Seg, SegStart, Tangents[i - 1], SegEnd, Tangents[i], false);
			}
		}
		else if (AccumDist + SegDist <= DrawData.SplitDistance)
		{
			// 세그먼트 전체가 이동 범위 안
			if (USplineMeshComponent* Seg = GetOrCreateSegment(NextIndex++))
			{
				SetupSegment(Seg, SegStart, Tangents[i - 1], SegEnd, Tangents[i], true);
			}
		}
		else
		{
			// 분기점 포함 세그먼트: Lerp로 분기 위치/탄젠트 계산
			const float T           = (SegDist > 0.f) ? ((DrawData.SplitDistance - AccumDist) / SegDist) : 0.f;
			const FVector SplitPos  = FMath::Lerp(SegStart, SegEnd, T);
			const FVector SplitTang = FMath::Lerp(Tangents[i - 1], Tangents[i], T);

			if (USplineMeshComponent* SegA = GetOrCreateSegment(NextIndex++))
			{
				SetupSegment(SegA, SegStart, Tangents[i - 1], SplitPos, SplitTang, true);
			}
			if (USplineMeshComponent* SegB = GetOrCreateSegment(NextIndex++))
			{
				SetupSegment(SegB, SplitPos, SplitTang, SegEnd, Tangents[i], false);
			}
		}

		AccumDist += SegDist;
	}

	// 이번 갱신에서 사용하지 않은 세그먼트 숨기기
	for (int32 i = NextIndex; i < SegmentPool.Num(); ++i)
	{
		if (IsValid(SegmentPool[i]))
		{
			SegmentPool[i]->SetVisibility(false);
		}
	}
}

USplineMeshComponent* UPBPathDisplayComponent::GetOrCreateSegment(int32 Index)
{
	if (SegmentPool.IsValidIndex(Index) && IsValid(SegmentPool[Index]))
	{
		return SegmentPool[Index];
	}

	if (SegmentPool.Num() >= MaxSegmentPoolSize)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PBPathDisplayComponent] 세그먼트 풀이 가득 찼습니다. (%s)"), *GetOwner()->GetName());
		return nullptr;
	}

	// VisualActor가 월드 원점에 위치하므로 world 좌표 = local 좌표
	USplineMeshComponent* NewSegment = NewObject<USplineMeshComponent>(VisualActor);
	NewSegment->SetMobility(EComponentMobility::Movable);
	NewSegment->RegisterComponent();
	NewSegment->AttachToComponent(VisualActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	SegmentPool.Add(NewSegment);
	return NewSegment;
}
