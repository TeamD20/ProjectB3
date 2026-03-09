// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PBAbilityGrantTypes.h"
#include "PBAbilitySetData.generated.h"

// 어빌리티 셋 정의 — 용도별로 DA 1개씩
// DA_Innate, DA_Class_Fighter, DA_Weapon_Longsword 등
UCLASS(BlueprintType)
class PROJECTB3_API UPBAbilitySetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 부여할 어빌리티 목록
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<FPBAbilityGrantEntry> Abilities;

	// 부여할 패시브 GE 목록
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<FPBEffectGrantEntry> PassiveEffects;
};
