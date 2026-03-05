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

	FPBPathDrawData DrawData;
	DrawData.BasePathPoints = PathPoints;
	BuildTerrainSnappedPoints(PathPoints, DrawData);
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
	if (MaxMoveDistance <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("[PBPathDisplayComponent] MaxMoveDistance가 0 이하입니다. (%.1f) (%s)"), MaxMoveDistance, *GetOwner()->GetName());
	}
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
	
	OutDrawData.PathPoints.Reset();
	OutDrawData.PathPoints.Add(NavPathPoints[0] + BaseOffsetVec);

	UWorld* World = GetWorld();

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

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(GetOwner());

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
					OutDrawData.SnappedCorrectionPoints.Add(SnappedPoint);
				}
			}
		}

		OutDrawData.PathPoints.Add(SegEnd + BaseOffsetVec);
	}
}

void UPBPathDisplayComponent::CalculateTotalDistance(FPBPathDrawData& InOutDrawData) const
{
	// TotalDistance는 보정 point를 포함하지 않고, NavigationSystem이 계산한 거리를 사용하기 위해 BasePathPoints 활용
	InOutDrawData.TotalDistance = 0.0f;
	for (int32 i = 1; i < InOutDrawData.BasePathPoints.Num(); ++i)
	{
		const float SegmentDist = FVector::Dist(InOutDrawData.BasePathPoints[i - 1], InOutDrawData.BasePathPoints[i]);
		InOutDrawData.TotalDistance += SegmentDist;
	}
}

void UPBPathDisplayComponent::CalculateSplitDistance(FPBPathDrawData& InOutDrawData) const
{
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
	for (const FVector& Point : InDrawData.SnappedCorrectionPoints)
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
		// 방향은 유지하되, 크기가 세그먼트 거리를 과도하게 초과하지 않도록 정규화 후 스케일링
		FVector SafeStartTangent = WStartTangent;
		if (!SafeStartTangent.IsNearlyZero())
		{
			SafeStartTangent = SafeStartTangent.GetSafeNormal() * (SegmentDistance * TangentTension);
		}
		FVector SafeEndTangent = WEndTangent;
		if (!SafeEndTangent.IsNearlyZero())
		{
			SafeEndTangent = SafeEndTangent.GetSafeNormal() * (SegmentDistance * TangentTension);
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
