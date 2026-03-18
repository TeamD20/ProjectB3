// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBSkillTooltipWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/**
 * 스킬바 슬롯에 마우스 오버 시 표시되는 스킬 툴팁 위젯
 */
UCLASS()
class PROJECTB3_API UPBSkillTooltipWidget : public UPBWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;

public:
	// 실제 뷰모델의 스킬 데이터를 받아 툴팁 UI를 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "UI | Tooltip")
	void SetTooltipData(const struct FPBSkillSlotData& InSlotData);

protected:
	// --- 바인딩 위젯 ---
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillLargeIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillTypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageDescText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DiceRollDescText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DiceRollIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillDescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ActionRangeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ActionRangeIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RollTypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RollTypeIcon;

	// 주사위 굴림 종류(명중, 내성 등)에 따른 아이콘 맵핑 리스트
	// 에디터 디테일 패널에서 디자이너가 직접 맵핑할 수 있습니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI | Tooltip", meta = (ForceInlineRow))
	TMap<EPBDiceRollType, TSoftObjectPtr<UTexture2D>> RollTypeIconMap;

protected:
	// --- 에디터 프리뷰용 더미 데이터 ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData")
	FText DummySkillName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData")
	FText DummySkillType = FText::FromString(TEXT("Null"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData")
	FText DummyDamageDesc;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData")
	FText DummyDiceRollDesc;

	// 주사위 텍스트 색상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData|Color")
	FSlateColor DummyDiceRollColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData")
	TObjectPtr<UTexture2D> DummyDiceRollIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData", meta=(MultiLine=true))
	FText DummyDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData")
	FText DummyActionRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData")
	FText DummyRollType;
};
