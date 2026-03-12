// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "ProjectB3/UI/SkillBar/PBSkillBarTypes.h"
#include "PBSkillIconWidget.generated.h"

class UImage;
class UTextBlock;
class UBorder;

/** 
 * [빨강 영역] 개별 스킬 아이콘 위젯 
 * 스킬바의 각 카테고리에 장착됩니다.
 */
UCLASS()
class PROJECTB3_API UPBSkillIconWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯 데이터를 기반으로 위젯의 시각적 요소(아이콘, 쿨다운, 포커스 등)를 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "UI|SkillIcon")
	void UpdateSlot(const FPBSkillSlotData& InSlotData);

protected:
	// 스킬 아이콘 이미지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillIcon;

	// 선택(Pressed) 시 나타나는 포커스 테두리
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> FocusBorder;

	// 쿨다운 텍스트 (필요 시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText;

	// 발동 불가 시 나타나는 오버레이 (필요 시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> DisabledOverlay;
};
