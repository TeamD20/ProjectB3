// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PBPathDisplayComponent.generated.h"

class USplineMeshComponent;
class UMaterialInterface;
class UStaticMesh;

// DrawDebugPath에 전달할 경로 렌더링 데이터
struct FPBPathDrawData
{
	// 선분 렌더링에 사용할 포인트 배열 (terrain-snap 보정 포인트 포함)
	TArray<FVector> PathPoints;
	// 전체 이동 경로 거리
	float TotalDistance  = 0.f;
	// InRange/OutOfRange 색상 분기 거리
	float SplitDistance = 0.f;
	// 원본 Nav Path 포인트
	TArray<FVector> BasePathPoints;
	// Terrain-snap 성공 보정 포인트
	TArray<FVector> SnappedCorrectionPoints;
};

/**
* 외부에서 전달받은 경로 포인트 배열을 실선으로 시각화하는 컴포넌트.
* 경로 탐색은 담당하지 않으며, 시각화만 책임진다.
* 이동 어빌리티 활성화 시에만 표시되어야 하며, 최대 이동 거리 기준으로 색상이 구분된다.
*/
UCLASS()
class PROJECTB3_API UPBPathDisplayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBPathDisplayComponent();

	// 경로 표시 활성화/비활성화.
	void SetPathDisplayEnabled(bool bEnabled);

	// 외부에서 쿼리된 경로 포인트를 받아 시각화
	void DisplayPath(const TArray<FVector>& PathPoints, bool bDisplayDistance);

	// 경로 시각화를 초기화하고 모든 세그먼트를 숨김
	void ClearPath();

	// 현재 경로 표시 활성화 여부를 반환
	bool IsPathDisplayEnabled() const { return bPathDisplayEnabled; }

	// 최대 이동 거리를 설정
	void SetMaxMoveDistance(float NewMaxMoveDistance) { MaxMoveDistance = NewMaxMoveDistance; }

	// 이동 시작 시 확정 경로를 전달받아 경로 추적 시작
	void BeginPathTracking(const TArray<FVector>& PathPoints);

	// 현재 위치 기준 남은 경로를 계산하여 표시 갱신 (Moving 모드 Tick에서 호출)
	void UpdateTrackedPath(const FVector& CurrentLocation);

	// 경로 추적 종료 및 표시 초기화
	void EndPathTracking();

protected:
	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	// 필수 프로퍼티 검증 및 로그
	void ValidateProperties();

	// PathPoints 배열을 기반으로 OutDrawData.PathPoints와 OutDrawData.SnappedCorrectionPoints를 삽입
	// 긴 세그먼트에 terrain-snapped 중간 보정 포인트를 생성
	void BuildTerrainSnappedPoints(const TArray<FVector>& NavPathPoints, FPBPathDrawData& OutDrawData) const;
	
	// OutDrawData.BasePathPoints를 기반으로 OutDrawData.TotalDistance를 계산
	void CalculateTotalDistance(FPBPathDrawData& InOutDrawData) const;
	
	// OutDrawData.PathPoints를 기반으로 OutDrawData.SplitDistance를 계산
	void CalculateSplitDistance(FPBPathDrawData& InOutDrawData) const;

	// 풀에서 Index번째 세그먼트를 가져오거나 새로 생성한다.
	USplineMeshComponent* GetOrCreateSegment(int32 Index);

	// SplineMeshComponent 풀로 경로 메시를 재구성한다.
	void RebuildLineSegments(const FPBPathDrawData& DrawData);

	// 풀의 모든 세그먼트를 숨김
	void HideAllSegments();

	// 거리 표시
	void DisplayDistance(const FPBPathDrawData& InDrawData) const;

	// Debug Line으로 경로를 시각화
	void DrawDebugPath(const FPBPathDrawData& InDrawData) const;

	// TrackedPathPoints에서 CurrentLocation 투영 이후의 잔여 경로 포인트 배열을 반환
	TArray<FVector> ExtractRemainingPath(const FVector& CurrentLocation) const;

public:
	/*~ PathDisplay Settings ~*/

	// 세그먼트 풀 최대 크기 (Nav 경로는 보통 5~20)
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	int32 MaxSegmentPoolSize = 60;

	// NavigationSystem이 계산한 경로 포인트에 적용할 Z 오프셋
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	float BasePathZOffset = -2.0f;

	// 보정 경로 포인트에 공통 적용할 Z 오프셋 (지형 매몰 방지 및 NaviMesh 높이와 맞추기 위함)
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	float CorrectionPathZOffset = 15.0f;
	
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	float TangentTension = 0.3f;

	
	/*~ Terrain Snap Settings ~*/

	// 이 수평 거리 이상인 세그먼트에만 terrain-snap 보정 포인트 생성
	UPROPERTY(EditAnywhere, Category = "PathDisplay|TerrainSnap")
	float TerrainSnapLengthThreshold = 50.0f;

	// 이 높이 차 이상인 세그먼트에만 terrain-snap 보정 포인트 생성
	UPROPERTY(EditAnywhere, Category = "PathDisplay|TerrainSnap")
	float TerrainSnapHeightThreshold = 10.0f;

	// Line Trace 시작 Z의 상향 여유 (cm). max(SegStart.Z, SegEnd.Z) 기준.
	UPROPERTY(EditAnywhere, Category = "PathDisplay|TerrainSnap")
	float TerrainTraceStartOffset = 10.0f;

	// Line Trace 종료 Z의 하향 여유 (cm). min(SegStart.Z, SegEnd.Z) 기준.
	UPROPERTY(EditAnywhere, Category = "PathDisplay|TerrainSnap")
	float TerrainTraceEndOffset = 10.0f;

	// Materials: 건물/나무 위에 렌더링되나, 캐릭터에는 가려져야 한다. (Custom Depth Stencil 활용)
	// 이동 범위 내 구간에 사용할 선 머티리얼
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	TObjectPtr<UMaterialInterface> InRangeMaterial;

	// 이동 범위 외 구간에 사용할 선 머티리얼
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	TObjectPtr<UMaterialInterface> OutOfRangeMaterial;

	// 선 세그먼트에 사용할 스태틱 메시 (얇은 튜브/실린더 형태)
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	TObjectPtr<UStaticMesh> LineMesh;

private:
	// 경로 시각화 전용 액터. SplineMeshComponent 풀을 소유
	UPROPERTY()
	TObjectPtr<AActor> VisualActor;

	// SplineMeshComponent 재사용 풀. 반복 생성/파괴를 방지
	UPROPERTY()
	TArray<TObjectPtr<USplineMeshComponent>> SegmentPool;

	// 현재 경로 표시 활성화 여부. SetPathDisplayEnabled로만 변경
	bool bPathDisplayEnabled = false;

	// 캐릭터의 최대 이동 가능 거리 (cm). SetMaxMoveDistance로만 변경
	float MaxMoveDistance = 0.0f;

	// 이동 중 추적할 확정 경로 포인트 (BeginPathTracking으로 설정)
	TArray<FVector> TrackedPathPoints;
};
