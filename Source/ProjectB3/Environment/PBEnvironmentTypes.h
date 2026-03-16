// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectKey.h"
#include "PBEnvironmentTypes.generated.h"

/**
 * 엄폐 수준. 미구현 기능이지만, 추후 확장을 위해 정의해둠.
 */
UENUM(BlueprintType)
enum class EPBCoverLevel : uint8
{
	// 엄폐 없음
	None           UMETA(DisplayName = "None"),
	// 반 엄폐 (AC +2)
	Half           UMETA(DisplayName = "Half"),
	// 3/4 엄폐 (AC +5)
	ThreeQuarter   UMETA(DisplayName = "Three Quarter"),
	// 완전 엄폐 (타겟팅 불가)
	Full           UMETA(DisplayName = "Full")
};

/**
 * 시야 판정 결과.
 */
USTRUCT(BlueprintType)
struct FPBLoSResult
{
	GENERATED_BODY()

	// 시야가 확보되는지
	UPROPERTY(BlueprintReadOnly)
	bool bHasLineOfSight = false;

	// 엄폐 수준
	UPROPERTY(BlueprintReadOnly)
	EPBCoverLevel CoverLevel = EPBCoverLevel::None;

	// 고도 차이 (양수 = 시전자가 높음, cm 단위)
	UPROPERTY(BlueprintReadOnly)
	float ElevationDelta = 0.0f;
};

/**
 * 경로 비용 결과.
 */
USTRUCT(BlueprintType)
struct FPBPathCostResult
{
	GENERATED_BODY()

	// 경로의 총 이동 비용 (= MP 소모량, cm 단위)
	UPROPERTY(BlueprintReadOnly)
	float TotalCost = 0.0f;

	// 경로 포인트 배열
	UPROPERTY(BlueprintReadOnly)
	TArray<FVector> PathPoints;

	// 유효한 경로가 존재하는지
	UPROPERTY(BlueprintReadOnly)
	bool bIsValid = false;
};

/**
 * LoS 캐시 키.
 * Source 좌표 + Target 액터 식별자로 구성.
 */
struct FPBLoSCacheKey
{
	// 시전자 위치 (월드 좌표, Tolerance 적용하여 양자화)
	FIntVector QuantizedSource;

	// 대상 액터 식별자
	FObjectKey TargetID;

	FPBLoSCacheKey() = default;

	FPBLoSCacheKey(const FVector& InSource, const AActor* InTarget, float Tolerance)
		: TargetID(InTarget)
	{
		// Tolerance 기준으로 좌표를 정수 격자에 스냅
		// 예: Tolerance = 50 → (123, 456, 789) → (2, 9, 15)
		const float InvTolerance = (Tolerance > 0.0f) ? (1.0f / Tolerance) : 1.0f;
		QuantizedSource = FIntVector(
			FMath::RoundToInt(InSource.X * InvTolerance),
			FMath::RoundToInt(InSource.Y * InvTolerance),
			FMath::RoundToInt(InSource.Z * InvTolerance)
		);
	}

	bool operator==(const FPBLoSCacheKey& Other) const
	{
		return QuantizedSource == Other.QuantizedSource && TargetID == Other.TargetID;
	}

	friend uint32 GetTypeHash(const FPBLoSCacheKey& Key)
	{
		return HashCombine(GetTypeHash(Key.QuantizedSource), GetTypeHash(Key.TargetID));
	}
};
