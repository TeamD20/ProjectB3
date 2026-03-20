// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBActionIndicatorTypes.generated.h"

class UTexture2D;

/**
 * 행동 인디케이터 종류
 */
UENUM(BlueprintType)
enum class EPBActionIndicatorType : uint8
{
    None,
    CombatStart,    // 전투 진입
    CombatEnd,      // 전투 종료
    SkillCast,      // 스킬 시전 중
    Moving,         // 이동 중 (위젯만 준비, 데이터 연동 미구현)
    Waiting,        // 대기
    Reaction,       // 반응 행동 대기
};

/**
 * 행동 인디케이터 데이터 스냅샷
 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBActionIndicatorData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionIndicator")
    EPBActionIndicatorType ActionType = EPBActionIndicatorType::None;

    // 표기할 텍스트 (예: "파이어볼 시전 중", "이동 중")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionIndicator")
    FText DisplayText;

    // 관련 아이콘
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionIndicator")
    TSoftObjectPtr<UTexture2D> Icon;

    // 활성 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionIndicator")
    bool bIsActive = false;
};
