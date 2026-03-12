// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PBCharacterStatsRow.generated.h"

/**
 * 캐릭터 기본 능력치 DataTable Row.
 * RowName = 캐릭터/클래스 식별자 (예: "Fighter", "Goblin")
 */
USTRUCT(BlueprintType)
struct FPBCharacterStatsRow : public FTableRowBase
{
	GENERATED_BODY()

	// 근력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Primary")
	float Strength = 10.f;

	// 민첩
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Primary")
	float Dexterity = 10.f;

	// 건강
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Primary")
	float Constitution = 10.f;

	// 지능
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Primary")
	float Intelligence = 10.f;

	// 클래스별 기본 최대 체력 (1레벨 기준)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secondary")
	float BaseMaxHP = 10.f;

	// 기본 방어력
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Secondary")
	float BaseArmorClass = 10.f;
};
