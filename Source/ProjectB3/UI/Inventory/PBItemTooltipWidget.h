// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBItemTooltipData.h"
#include "PBItemTooltipWidget.generated.h"

class UImage;
class UTextBlock;
struct FPBItemInstance;

class UPBInventoryViewModel;

/**
 * 인벤토리 아이템 위젯에 오버랩 시 팝업되어, 
 * 아이템 이름, 피해량, 어빌리티, 서사 등을 3개의 박스로 보여주는 툴팁 위젯.
 */
UCLASS()
class PROJECTB3_API UPBItemTooltipWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// ViewModel을 바인딩하고 툴팁 데이터 변경 이벤트를 구독
	UFUNCTION(BlueprintCallable, Category = "UI|Tooltip")
	void BindViewModel(UPBInventoryViewModel* InViewModel);

	// 미리 조립된 툴팁 스냅샷을 받아 UI 갱신 (에디터 프리뷰 대응용)
	UFUNCTION(BlueprintCallable, Category = "UI|Tooltip")
	void SetTooltipData(const FPBItemTooltipData& InData);

protected:
	virtual void NativePreConstruct() override;

	// ==== 바인딩 위젯: 공통 ====
	
	// 툴팁 하단에 고정으로 표시되는 반투명 보라빛 장식 등
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TooltipBottomDecor;

	// ==== 바인딩 위젯: ====
	
	// 박스 1 배경에 깔리는 등급별 고유 색상 오버레이
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Box1_RarityOverlay;

	// 무기/방어구 전용 스탯들을 묶어두는 래퍼 박스 (소모품일 때 통째로 숨김 처리용)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Box_Equipment;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RarityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageRangeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DiceIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DiceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ModifierText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DamageTypeIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageTypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemModelImage;

	// ==== 바인딩 위젯: 박스 2 (어빌리티) ====
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Box_Ability;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AbilityDescText;

	// ==== 바인딩 위젯: 박스 3 (로어) ====
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Box_Lore;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> LoreIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> LoreIcon_Consumable;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LoreDescText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCategoryText;

	// ==== 바인딩 위젯: 소모품 전용 ====
	
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Box_Consumable;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ConsumableEffectText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ConsumableEffectIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DurationText;

	// ==== 에디터 및 설정 데이터 ====
	
	// 블루프린트 디자이너 지정용: 등급별 Box1_RarityOverlay 컬러 매핑
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Rarity", meta = (ForceInlineRow))
	TMap<EPBItemRarity, FLinearColor> RarityOverlayColorMap;

	// 에디터 뷰포트에서 등급 오버레이 색상을 테스트해보기 위한 더미 등급 (텍스트는 덮어쓰지 않고 색상만 미리보기)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Rarity")
	EPBItemRarity PreviewRarity = EPBItemRarity::Common;

	// 블루프린트 디자이너 지정용: 최하단 고정 장식 텍스처 (반투명 보라빛 효과 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Decor")
	TSoftObjectPtr<UTexture2D> BottomDecorTexture;

	// ==== 빈 데이터 대체 (Fallback) 텍스트 ====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Text")
	FText FallbackItemName = FText::FromString(TEXT("데이터에 존재하지 않는 아이템"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Text")
	FText FallbackRarity = FText::FromString(TEXT("미정"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Text")
	FText FallbackDamageRange = FText::FromString(TEXT("Null"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Text")
	FText FallbackDice = FText::FromString(TEXT("Null"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Text")
	FText FallbackDamageType = FText::FromString(TEXT("Null"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Text")
	FText FallbackAbilityDesc = FText::FromString(TEXT("Null"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Text")
	FText FallbackLoreDesc = FText::FromString(TEXT("Null"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Text")
	FText FallbackItemCategory = FText::FromString(TEXT("Null"));

	// ==== 빈 데이터 대체 (Fallback) 이미지 ====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Image")
	TSoftObjectPtr<UTexture2D> FallbackDiceIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Image")
	TSoftObjectPtr<UTexture2D> FallbackDamageTypeIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Image")
	TSoftObjectPtr<UTexture2D> FallbackItemModelIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Fallback Image")
	TSoftObjectPtr<UTexture2D> FallbackLoreIcon;

	// (Option) 에디터에서 볼 더미 데이터 (주로 블루프린트 구조체 기본값 테스트용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DummyData")
	FPBItemTooltipData DummyTooltipData;
};
