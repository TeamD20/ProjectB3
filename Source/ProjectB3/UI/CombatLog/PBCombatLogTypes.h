// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "PBCombatLogTypes.generated.h"

/** 컴뱃 로그 항목의 분류 타입 */
UENUM(BlueprintType)
enum class EPBCombatLogType : uint8
{
	// 일반 피해
	Damage		UMETA(DisplayName = "Damage"),
	// 치명타 피해
	CritDamage	UMETA(DisplayName = "Critical Damage"),
	// 치유
	Heal		UMETA(DisplayName = "Heal"),
	// 회피/빗나감
	Miss		UMETA(DisplayName = "Miss"),
	// 내성 성공
	SaveSuccess	UMETA(DisplayName = "Save Success"),
	// 내성 실패
	SaveFailed	UMETA(DisplayName = "Save Failed"),
	// 버프/디버프 부여 또는 해제
	Status		UMETA(DisplayName = "Status"),
	// 사망
	Death		UMETA(DisplayName = "Death"),
	// 턴 시작, 라운드 등 시스템 메시지
	System		UMETA(DisplayName = "System"),
};

/** 컴뱃 로그 단일 항목 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBCombatLogEntry
{
	GENERATED_BODY()

	// 표시할 로그 텍스트 (포맷팅 완료 상태)
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FText LogText;

	// 로그 분류 타입
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	EPBCombatLogType LogType = EPBCombatLogType::System;

	// 렌더링 색상 (ViewModel에서 타입별 색상 매핑으로 결정)
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	FLinearColor TextColor = FLinearColor::White;

	// 전투 내 라운드 번호 (0 = 전투 시작 전)
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 Round = 0;

	// 라운드 내 턴 인덱스
	UPROPERTY(BlueprintReadOnly, Category = "CombatLog")
	int32 TurnIndex = 0;
};

/** GameplayTag → 표시 이름 / 아이콘 매핑 DataTable 행 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBGameplayTagDisplayRow : public FTableRowBase
{
	GENERATED_BODY()

	// 매핑 대상 Gameplay Tag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TagDisplay")
	FGameplayTag Tag;

	// UI에 표시될 이름 (예: "중독", "화상", "축복")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TagDisplay")
	FText DisplayName;

	// UI 아이콘 (상태 아이콘, 로그 접두 아이콘 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TagDisplay")
	TSoftObjectPtr<UTexture2D> Icon;

	// 로그 색상 오버라이드 (투명 = 타입별 기본 색상 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TagDisplay")
	FLinearColor ColorOverride = FLinearColor::Transparent;
};
