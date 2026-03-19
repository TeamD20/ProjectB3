// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBItemTooltipData.generated.h"

class UTexture2D;

/**
 * 인벤토리 아이템 툴팁 위젯 표시를 위한 일회성 스냅샷 데이터.
 * ViewModel ↔ Widget 사이의 데이터 전달 규격입니다.
 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBItemTooltipData
{
	GENERATED_BODY()

	// 아이템 이름
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText ItemName;

	// 등급 텍스트 (예: "희귀")
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText RarityText;

	// 배경 등급별 오버레이 색상
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FLinearColor RarityOverlayColor = FLinearColor::Transparent;

	// 피해 범위 텍스트 (예: "4~9 피해")
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText DamageRangeText;

	// 주사위 표현 텍스트 (예: "1d6")
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText DiceText;

	// 주사위 텍스트/표시 색상
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FSlateColor DiceColor = FSlateColor(FLinearColor::White);

	// 고정 보정치 표기 (예: "(+3)")
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText ModifierText;

	// 피해 유형 텍스트
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText DamageTypeText;

	// 피해 유형 태그 (확장용)
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FGameplayTag DamageTypeTag;

	// 주사위 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	TSoftObjectPtr<UTexture2D> DiceIcon;

	// 피해 유형 앞쪽 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	TSoftObjectPtr<UTexture2D> DamageTypeIcon;

	// 방어구/장신구 ---
	
	// 방어도 수치
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	int32 ArmorClass = 0;

	// 방어구 분류 (예: "경갑")
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText ArmorTypeText;

	// 은신 불이익 표시 텍스트
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText StealthWarningText;

	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	bool bStealthDisadvantage = false;

	// 방패 등 AC 관련 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	TSoftObjectPtr<UTexture2D> ArmorClassIcon;

	// --- 박스 1: 소모품 ---
	
	// 효과 텍스트 (예: "3d4+3 회복")
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText ConsumableEffectText;

	// 효과 앞 아이콘 (포션 모양 등)
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	TSoftObjectPtr<UTexture2D> ConsumableEffectIcon;

	// 지속시간 또는 소모성 문구 (예: "긴 휴식 전까지")
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	FText DurationText;

	// --- 박스 1: 공통 아이콘 ---

	// 3D 아이템 모형 / 고해상도 미리보기 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box1")
	TSoftObjectPtr<UTexture2D> ItemModelIcon;

	// --- 구조체 편의 데이터 ---
	
	// 위젯에서 동적 가시성 처리를 위해 분류 값을 함께 넘김
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|General")
	EPBItemType ItemType = EPBItemType::Misc;

	// 어빌리티 설명 텍스트
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box2")
	FText AbilityDescription;

	// 배경 설명 헤더에 등장하는 장식 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box3")
	TSoftObjectPtr<UTexture2D> LoreIcon;

	// 아이템 배경/서사 텍스트
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box3")
	FText LoreDescription;

	// 아이템 종류/분류 (예: "철퇴", "경갑", "반지")
	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Box3")
	FText ItemCategoryText;

	// --- 가시성 제어용 Helper 판별 ---

	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Visibility")
	bool bHasAbility = false;

	UPROPERTY(BlueprintReadOnly, Category = "Tooltip|Visibility")
	bool bHasLore = false;
};
