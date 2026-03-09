// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBAbilityTypes.generated.h"

/** 어빌리티 타겟팅 모드 */
UENUM(BlueprintType)
enum class EPBTargetingMode : uint8
{
	// 타겟 지정 불필요 (Dash 등)
	None,

	// 자기 자신 대상 (자가 버프, 치유 등)
	Self,

	// 액터 1명 지정 (근접/원거리 공격, 단일 대상 주문)
	SingleTarget,

    // 액터 다수 지정
	MultiTarget,

	// 지면 위치 지정, 범위 표시 없음 (텔레포트, 소환 등)
	Location,

	// 위치 지정 + 범위 표시 (파이어볼, 폭발 화살 등)
	AoE
};

/** 어빌리티 자원 소모 유형 */
UENUM(BlueprintType)
enum class EPBAbilityCostType : uint8
{
	// 주 행동
	Action,

	// 보조 행동
	BonusAction,

	// 반응 행동 (상대 턴에 사용)
	Reaction,

	// 자원 소모 없음
	Free
};

/** 어빌리티 실행에 필요한 타겟 정보. AI/플레이어 양쪽 경로에서 공통 사용. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBAbilityTargetData
{
	GENERATED_BODY()

	// 타겟팅 모드 (수신 측에서 데이터 해석 기준)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	EPBTargetingMode TargetingMode = EPBTargetingMode::None;

	// 사전 지정 타겟 액터 목록 (Self / SingleTarget / MultiTarget 용)
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> TargetActors;

	// 타겟 위치 (Location, AoE 용)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	FVector TargetLocation = FVector::ZeroVector;

	// AoE 반경
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	float AoERadius = 0.f;

	// AoE 히트 결과 — 실행 단계에서 채워짐 (TargetActors와 의미가 다르므로 분리 유지)
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;

	// 단일 타겟 반환 (SingleTarget, Self 용). 유효하지 않으면 nullptr 반환.
	AActor* GetSingleTarget() const
	{
		return (TargetActors.Num() > 0) ? TargetActors[0].Get() : nullptr;
	}

	// Self 타겟 반환 (GetSingleTarget alias)
	AActor* GetSelfTarget() const
	{
		return GetSingleTarget();
	}

	// 전체 타겟 액터 배열 반환 (MultiTarget 용)
	TArray<AActor*> GetAllTargets() const
	{
		TArray<AActor*> Result;
		for (const TWeakObjectPtr<AActor>& Weak : TargetActors)
		{
			if (Weak.IsValid())
			{
				Result.Add(Weak.Get());
			}
		}
		return Result;
	}

	// 타겟팅 모드 기반 유효성 검증
	bool IsValid() const
	{
		switch (TargetingMode)
		{
		case EPBTargetingMode::None:         return true;
		case EPBTargetingMode::Self:         return TargetActors.Num() > 0 && TargetActors[0].IsValid();
		case EPBTargetingMode::SingleTarget: return TargetActors.Num() > 0 && TargetActors[0].IsValid();
		case EPBTargetingMode::MultiTarget:  return TargetActors.Num() > 0;
		case EPBTargetingMode::Location:     return !TargetLocation.IsZero();
		case EPBTargetingMode::AoE:          return !TargetLocation.IsZero() && AoERadius > 0.f;
		default:                             return false;
		}
	}
};

class UPBGameplayAbility;

/** PlayerController가 UPBTargetingComponent에 타겟팅 세션을 요청할 때 전달하는 구조체. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBTargetingRequest
{
	GENERATED_BODY()

	// 요청한 어빌리티 (사거리 검증 위임용)
	TWeakObjectPtr<UPBGameplayAbility> RequestingAbility;

	// 타겟팅 모드
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	EPBTargetingMode Mode = EPBTargetingMode::None;

	// 발동자 위치 (사거리 계산 기준)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	FVector OriginLocation = FVector::ZeroVector;

	// AoE 반경 (AoE 모드 미리보기용)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	float AoERadius = 0.f;

	// MultiTarget 최대 선택 수 (0이면 무제한)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	int32 MaxTargetCount = 1;
};

