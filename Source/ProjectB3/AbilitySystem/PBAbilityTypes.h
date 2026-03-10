// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectB3/PBGameplayTags.h"
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
enum class EPBAbilityType : uint8
{
	None,
	
	// 주 행동
	Action,

	// 보조 행동
	BonusAction,

	// 반응 행동 (상대 턴에 사용)
	Reaction,

	// 자원 소모 없음
	Free
};

/** 태그 컨테이너에서 Ability.Type.* 태그를 EPBAbilityType으로 변환. 미매칭 시 None 반환. */
inline EPBAbilityType GetAbilityTypeFromTags(const FGameplayTagContainer& Tags)
{
	if (Tags.HasTag(PBGameplayTags::Ability_Type_Action))      { return EPBAbilityType::Action; }
	if (Tags.HasTag(PBGameplayTags::Ability_Type_BonusAction)) { return EPBAbilityType::BonusAction; }
	if (Tags.HasTag(PBGameplayTags::Ability_Type_Reaction))    { return EPBAbilityType::Reaction; }
	if (Tags.HasTag(PBGameplayTags::Ability_Type_Free))        { return EPBAbilityType::Free; }
	return EPBAbilityType::None;
}

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

	// 타겟 위치 목록 (Location / AoE 용, 다수 지점 지정 가능)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	TArray<FVector> TargetLocations;

	// AoE 반경
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	float AoERadius = 0.f;

	// AoE 히트 결과 — 실행 단계에서 채워짐 (TargetActors와 의미가 다르므로 분리 유지)
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;

	// 단일 타겟 액터 반환 (SingleTarget, Self 용). 유효하지 않으면 nullptr 반환.
	AActor* GetSingleTargetActor() const
	{
		return (TargetActors.Num() > 0) ? TargetActors[0].Get() : nullptr;
	}

	// Self 타겟 액터 반환 (GetSingleTargetActor alias)
	AActor* GetSelfTargetActor() const
	{
		return GetSingleTargetActor();
	}

	// 전체 타겟 액터 배열 반환 (MultiTarget 용)
	TArray<AActor*> GetAllTargetActors() const
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

	// 단일 타겟 위치 반환. 없으면 ZeroVector 반환.
	FVector GetSingleTargetLocation() const
	{
		return (TargetLocations.Num() > 0) ? TargetLocations[0] : FVector::ZeroVector;
	}

	// 전체 타겟 위치 배열 반환 (Location / AoE / bAllowGroundTarget 용)
	const TArray<FVector>& GetAllTargetLocations() const
	{
		return TargetLocations;
	}

	// 액터 또는 위치 타겟이 하나라도 존재하는지 확인
	bool HasTarget() const
	{
		return TargetActors.Num() > 0 || TargetLocations.Num() > 0;
	}

	// 타겟팅 모드 기반 유효성 검증
	bool IsValid() const
	{
		switch (TargetingMode)
		{
		case EPBTargetingMode::None:         return true;
		case EPBTargetingMode::Self:         return TargetActors.Num() > 0 && TargetActors[0].IsValid();
		case EPBTargetingMode::SingleTarget: return (TargetActors.Num() > 0 && TargetActors[0].IsValid()) || TargetLocations.Num() > 0;
		case EPBTargetingMode::MultiTarget:  return TargetActors.Num() > 0;
		case EPBTargetingMode::Location:     return TargetLocations.Num() > 0;
		case EPBTargetingMode::AoE:          return TargetLocations.Num() > 0 && AoERadius > 0.f;
		default:                             return false;
		}
	}
};


class UPBGameplayAbility_Targeted;

/** PlayerController가 UPBTargetingComponent에 타겟팅 세션을 요청할 때 전달하는 구조체. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBTargetingRequest
{
	GENERATED_BODY()

	// 요청한 어빌리티 (사거리 검증 위임용)
	TWeakObjectPtr<UPBGameplayAbility_Targeted> RequestingAbility;

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

	// 액터 미지정 시 지면 위치 타겟 허용 (SingleTarget / MultiTarget 모드에서 유효)
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Targeting")
	bool bAllowGroundTarget = false;
};

