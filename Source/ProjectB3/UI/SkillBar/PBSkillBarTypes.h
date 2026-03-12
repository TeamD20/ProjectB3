// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBSkillBarTypes.generated.h"

class UTexture2D;

/** 스킬바 탭 식별용 열거형 */
UENUM(BlueprintType)
enum class EPBSkillBarTab : uint8
{
	Common UMETA(DisplayName = "Common"),
	Class UMETA(DisplayName = "Class"),
	Item UMETA(DisplayName = "Item"),
	Passive UMETA(DisplayName = "Passive"),
	Custom UMETA(DisplayName = "Custom")
};

/** 스킬바 슬롯 1개의 표시/상태 스냅샷 데이터 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBSkillSlotData
{
	GENERATED_BODY()

	// 바인딩된 어빌리티 핸들
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
	FGameplayAbilitySpecHandle AbilityHandle;

	// 표시 이름
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
	FText DisplayName;

	// 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
	TSoftObjectPtr<UTexture2D> Icon;

	// 행동 자원 소모 유형
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
	EPBAbilityType AbilityType = EPBAbilityType::None;

	// 잔여 쿨다운 (턴)
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
	int32 CooldownRemaining = 0;

	// 현재 발동 가능 여부
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
	bool bCanActivate = true;
};
