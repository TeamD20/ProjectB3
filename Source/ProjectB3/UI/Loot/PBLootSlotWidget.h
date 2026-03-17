// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBLootSlotWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
class UPBLootViewModel;

/**
 * 루팅 패널의 단일 슬롯 위젯.
 * 클릭 시 LootViewModel->TakeItem()을 호출하여 아이템을 획득한다.
 */
UCLASS()
class PROJECTB3_API UPBLootSlotWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	/** 슬롯 인덱스와 LootViewModel을 바인딩 */
	void InitializeSlot(int32 InSlotIndex, UPBLootViewModel* InViewModel);

	/** 현재 슬롯 데이터를 기반으로 UI를 갱신 */
	UFUNCTION(BlueprintCallable, Category = "UI|Loot")
	void RefreshSlot();

protected:
	/*~ UUserWidget Interface ~*/
	/** 버튼 이벤트 바인딩 */
	virtual void NativeConstruct() override;

	/** 버튼 이벤트 해제 */
	virtual void NativeDestruct() override;

private:
	/** 슬롯 클릭 시 아이템 획득 */
	UFUNCTION()
	void OnSlotClicked();

protected:
	// 아이템 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	// 스택 수량 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StackCountText;

	// 등급 테두리
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RarityBorder;

	// 장착 중 표시 오버레이 (bIsEquipped == true 시 표시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EquippedBadge;

	// 슬롯 클릭 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

private:
	// 바인딩된 슬롯 인덱스
	int32 SlotIndex = INDEX_NONE;

	// LootViewModel 약참조
	TWeakObjectPtr<UPBLootViewModel> LootViewModel;
};
