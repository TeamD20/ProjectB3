// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NavigationPath.h"
#include "PBEnvironmentTypes.h"
#include "PBEnvironmentSubsystem.generated.h"

class UPBLineOfSightStrategy;
class AController;

/**
 * AI와 플레이어가 공유하는 전술 환경 판정 서브시스템.
 * 경로 거리 산정, 시야(LoS) 판정을 단일 시스템에서 관리한다.
 * LoS 알고리즘은 전략 패턴으로 주입 가능하다.
 */
UCLASS()
class PROJECTB3_API UPBEnvironmentSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/*~ USubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* === 시야 판정 === */

	// 두 위치 간 시야 확인 (캐시 활성 시 캐시 우선 조회)
	UFUNCTION(BlueprintCallable, Category = "Environment|LineOfSight")
	FPBLoSResult CheckLineOfSight(const FVector& Source, const AActor* Target) const;

	// 특정 위치에서 사거리 내 모든 가시 대상 조회
	UFUNCTION(BlueprintCallable, Category = "Environment|LineOfSight")
	TArray<AActor*> GetVisibleTargetsFrom(const FVector& Position, float MaxRange, const TArray<AActor*>& CandidateTargets) const;

	// LoS 전략 교체
	UFUNCTION(BlueprintCallable, Category = "Environment|LineOfSight")
	void SetLoSStrategy(TSubclassOf<UPBLineOfSightStrategy> StrategyClass);

	/* === 경로 거리 === */

	// 주어진 경로 포인트 배열의 총 거리 계산
	UFUNCTION(BlueprintCallable, Category = "Environment|Path")
	float CalculatePathDistance(const TArray<FVector>& PathPoints) const;

	// 두 위치 간 NavSystem 기반 경로 비용 조회
	UFUNCTION(BlueprintCallable, Category = "Environment|Path")
	FPBPathFindResult CalculatePath(const FVector& Start, const FVector& End) const;

	// 컨트롤러 네비 에이전트 조건을 반영한 경로 비용 조회
	UFUNCTION(BlueprintCallable, Category = "Environment|Path")
	FPBPathFindResult CalculatePathForAgent(const AController* Controller, const FVector& GoalLocation, bool bAllowPartialPath = false) const;

	// 컨트롤러 기준 이동 요청 (경로 계산 조건과 이동 실행 조건 일치)
	UFUNCTION(BlueprintCallable, Category = "Environment|Path")
	bool RequestMoveToLocation(AController* Controller, const FVector& GoalLocation, float AcceptanceRadius = 50.f, bool bAllowPartialPath = false) const;

	// 경로 포인트 배열에서 특정 위치까지의 실제 이동 거리 계산
	UFUNCTION(BlueprintCallable, Category = "Environment|Path")
	float CalculateDistanceAlongPath(const TArray<FVector>& PathPoints, const FVector& CurrentLocation) const;

	// 이동력 기준 경로 클램핑 — 최종 목적지 반환, OutClampedPath에 클램핑된 경로 저장
	UFUNCTION(BlueprintCallable, Category = "Environment|Path")
	FVector CalculateClampedDestination(const TArray<FVector>& PathPoints, float MaxDistance, TArray<FVector>& OutClampedPath) const;

	/* === 위험 영역 판정 === */

	// 위험 영역 등록 (DamageArea BeginPlay에서 호출)
	void RegisterDamageArea(APBDamageArea* Area);

	// 위험 영역 해제 (DamageArea EndPlay에서 호출)
	void UnregisterDamageArea(APBDamageArea* Area);

	// 특정 포인트가 위험 영역 이내인지 쿼리 (캐시 활성 시 캐시 우선 조회)
	UFUNCTION(BlueprintCallable, Category = "Environment|Hazard")
	FPBHazardQueryResult QueryHazardAtPoint(const FVector& Point) const;

	/* === 캐시 수명 관리 === */

	// 캐시 세션 시작 — 이후 판정 결과를 캐싱
	UFUNCTION(BlueprintCallable, Category = "Environment|Cache")
	void BeginEnvironmentCache();

	// 캐시 세션 종료 — 캐시 데이터 전체 파기
	UFUNCTION(BlueprintCallable, Category = "Environment|Cache")
	void EndEnvironmentCache();

	// 캐시 활성 여부 조회
	UFUNCTION(BlueprintCallable, Category = "Environment|Cache")
	bool IsLoSCacheActive() const { return bEnvironmentCacheActive; }

private:
	// 현재 LoS 전략
	UPROPERTY()
	TObjectPtr<UPBLineOfSightStrategy> LoSStrategy;

	// LoS 결과 캐시 — 캐시 세션 동안만 유효
	mutable TMap<FPBLoSCacheKey, FPBLoSResult> LoSCache;

	// 경로 비용 캐시 (점 배열)
	mutable TMap<uint64, FPBPathFindResult> PathCostCache;

	// NavPath 캐시 — RequestMoveToLocation에서 FindPathSync 재호출 없이 재사용
	mutable TMap<uint64, FNavPathSharedPtr> NavPathCache;

	// 위험 영역 쿼리 캐시 — 양자화 좌표 기준
	mutable TMap<FIntVector, FPBHazardQueryResult> HazardCache;

	// 등록된 위험 영역 목록
	TArray<TWeakObjectPtr<APBDamageArea>> RegisteredDamageAreas;

	// 캐시 활성 상태
	bool bEnvironmentCacheActive = false;

	// Source 좌표 동일 판정 허용 오차 (기본 50cm)
	float LoSCacheTolerance = 50.0f;
};
