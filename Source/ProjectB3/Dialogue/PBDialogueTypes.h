// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBDialogueTypes.generated.h"

/** 화자(참여자) 표시 정보. Manager가 노드의 ParticipantTag를 기반으로 구성한다. */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBDialogueParticipantDisplayInfo
{
    GENERATED_BODY()

    // 화자 이름
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FText ParticipantName;

    // 이름 색상
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FLinearColor ParticipantColor = FLinearColor::White;

    // 참여자 식별 태그
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FGameplayTag ParticipantTag;
};

/** 선택지 1개의 표시 정보 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBDialogueChoiceInfo
{
    GENERATED_BODY()

    // 선택지 텍스트
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FText ChoiceText;

    // 선택 가능 여부 (조건 미충족 시 false)
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    bool bAvailable = true;

    // 비활성 사유 ("지능 12 필요" 등. bAvailable == false 시에만 유효)
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FText UnavailableReason;

    // 스킬 체크 연관 태그 (없으면 Invalid)
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FGameplayTag SkillCheckTag;

    // 필요 난이도 (0이면 체크 없음)
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    int32 RequiredDC = 0;
};

/** 주사위 굴리기/결과 표시 정보 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBDiceRollDisplayInfo
{
    GENERATED_BODY()

    // 체크 스킬명 ("설득", "위협" 등)
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FText SkillName;

    // 목표 난이도
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    int32 DC = 0;

    // 능력치 수정치
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    int32 Modifier = 0;

    // 굴림 결과 (1~20)
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    int32 RollResult = 0;

    // 합계 (RollResult + Modifier)
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    int32 TotalResult = 0;

    // 성공 여부
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    bool bSuccess = false;

    // 자연 20 여부
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    bool bNatural20 = false;
};
