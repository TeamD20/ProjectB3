// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPathDisplayComponent.h"
#include "Components/SplineComponent.h"
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

	if (IsValid(GetOwner()) && IsValid(GetOwner()->GetRootComponent()))
	{
		// PathSpline 생성 및 오너 액터에 등록
		PathSpline = NewObject<USplineComponent>(GetOwner(), TEXT("PathSpline"));
		PathSpline->RegisterComponent();
		PathSpline->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	}
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
	DrawDebugPath(DrawData);
	
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
	const FVector ZOffsetVec(0.f, 0.f, PathZOffset);

	OutDrawData.PathPoints.Reset();
	OutDrawData.PathPoints.Add(NavPathPoints[0] + ZOffsetVec);

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
					const FVector SnappedPoint = HitResult.ImpactPoint + ZOffsetVec;
					OutDrawData.PathPoints.Add(SnappedPoint);
					OutDrawData.SnappedCorrectionPoints.Add(SnappedPoint);
				}
			}
		}

		OutDrawData.PathPoints.Add(SegEnd + ZOffsetVec);
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

	float AccumulatedDist = 0.0f;
	for (int32 i = 1; i < InDrawData.PathPoints.Num(); ++i)
	{
		const FVector& SegStart = InDrawData.PathPoints[i - 1];
		const FVector& SegEnd   = InDrawData.PathPoints[i];
		const float SegDist     = FVector::Dist(SegStart, SegEnd);

		if (AccumulatedDist >= InDrawData.SplitDistance)
		{
			// 세그먼트 전체가 이동 범위 밖
			DrawDebugLine(World, SegStart, SegEnd, OutOfRangeColor, true, -1.0f, 0, LineThickness);
		}
		else if (AccumulatedDist + SegDist <= InDrawData.SplitDistance)
		{
			// 세그먼트 전체가 이동 범위 안
			DrawDebugLine(World, SegStart, SegEnd, InRangeColor, true, -1.0f, 0, LineThickness);
		}
		else
		{
			// 세그먼트 중간에 분기점이 위치할 경우 두 조각으로 분할
			const float RemainingInRange = InDrawData.SplitDistance - AccumulatedDist;
			const float T = (SegDist > 0.0f) ? (RemainingInRange / SegDist) : 0.0f;
			const FVector SplitPoint = FMath::Lerp(SegStart, SegEnd, T);

			DrawDebugLine(World, SegStart, SplitPoint, InRangeColor,    true, -1.0f, 0, LineThickness);
			DrawDebugLine(World, SplitPoint, SegEnd,   OutOfRangeColor, true, -1.0f, 0, LineThickness);
		}

		AccumulatedDist += SegDist;
	}

	// 기본 포인트 시각화: 파랑
	for (const FVector& Point : InDrawData.BasePathPoints)
	{
		DrawDebugSphere(World, Point + FVector(0.f, 0.f, PathZOffset), 7.f, 8, FColor::Blue, true, -1.f);
	}
	// 보정 포인트 시각화: 초록
	for (const FVector& Point : InDrawData.SnappedCorrectionPoints)
	{
		DrawDebugSphere(World, Point, 5.f, 6, FColor::Green, true, -1.f);
	}
}


