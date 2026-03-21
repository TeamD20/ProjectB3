// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "GameplayTagContainer.h"

#include "PBCharacterTypes.generated.h"

/**
 * 캐릭터 전투 식별 정보. 진영, 표시 이름, 초상화를 묶어 관리한다.
 */
USTRUCT(BlueprintType)
struct FPBCharacterIdentity
{
	GENERATED_BODY()

	// 캐릭터 식별 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	FGameplayTag IdentityTag;
	
	// 직업 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character", meta = (Categories = "Character.Class"))
	FGameplayTag ClassTag;
	
	// 진영 태그 (Player/Enemy/Neutral)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character", meta = (Categories = "Combat.Faction"))
	FGameplayTag FactionTag;

	// UI 표시 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	FText DisplayName;

	// UI 초상화
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
	TSoftObjectPtr<UTexture2D> Portrait;
};