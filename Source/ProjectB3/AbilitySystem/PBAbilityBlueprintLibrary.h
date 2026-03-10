// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PBAbilityTypes.h"
#include "PBAbilityBlueprintLibrary.generated.h"

/** 어빌리티 관련 유틸리티 라이브러리 */
UCLASS()
class PROJECTB3_API UPBAbilityBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 단일 타겟 액터 반환 (SingleTarget, Self 용). 유효하지 않으면 nullptr.
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static AActor* GetSingleTargetActor(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 전체 타겟 액터 배열 반환 (MultiTarget 용)
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static TArray<AActor*> GetAllTargetActors(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 단일 타겟 위치 반환. 없으면 ZeroVector.
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FVector GetSingleTargetLocation(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 전체 타겟 위치 배열 반환
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static TArray<FVector> GetAllTargetLocations(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 액터 또는 위치 타겟이 하나라도 존재하는지 확인
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static bool HasTarget(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 타겟팅 모드 기반 유효성 검증
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData", DisplayName = "Is Valid (Target Data)")
	static bool IsTargetDataValid(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 전체 AoE 히트 액터 배열 반환
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static TArray<AActor*> GetAllHitActors(UPARAM(ref) const FPBAbilityTargetData& TargetData);
};
