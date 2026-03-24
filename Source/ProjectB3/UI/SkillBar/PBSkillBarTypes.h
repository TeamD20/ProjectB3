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
	Passive UMETA(DisplayName = "Passive")
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

	// 현재 선택(Pressed) 여부 (포커스 테두리 표시용)
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
	bool bIsActive = false;

	// 소비 아이템 식별자 (어빌리티 대신 아이템 사용 시)
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar")
	FGuid ItemInstanceID;

	/* ============================================== */
	/* 툴팁 전용 데이터 (Tooltip Data)                 */
	/* ============================================== */
	
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	FText SkillType;

	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	FText DamageDesc;

	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	FText DiceRollDesc;

	// 주사위 텍스트 색상
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	FSlateColor DiceRollColor;

	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	TSoftObjectPtr<UTexture2D> DiceRollIcon;

	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	FText ActionRange;

	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	FText RollType;

	// 주사위 굴림 종류 (명중, 내성 등)에 따른 아이콘 맵핑용
	UPROPERTY(BlueprintReadOnly, Category = "SkillBar|Tooltip")
	EPBDiceRollType RollTypeEnum = EPBDiceRollType::None;
};

/** 장비/아이템 슬롯 1개의 스냅샷 데이터 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBEquipmentSlotData
{
	GENERATED_BODY()

	// 아이템 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TSoftObjectPtr<UTexture2D> Icon;

	// 수량 (소모품인 경우)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	int32 Quantity = 0;

	// 장착/사용 가능 여부
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsAvailable = true;

	// 아이템 이름이나 추가 정보용 구조체가 필요하면 여기에 확장
};
