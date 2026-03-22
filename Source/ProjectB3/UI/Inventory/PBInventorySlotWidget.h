// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBInventorySlotWidget.generated.h"

// 슬롯 우클릭 시 컨텍스트 메뉴 표시 요청 델리게이트
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnSlotContextMenuRequestedSignature,
	const FGuid&  /*InstanceID*/,
	EPBItemType   /*ItemType*/,
	FVector2D     /*ScreenPosition*/)

class UBorder;
class UButton;
class UDragDropOperation;
class UImage;
class UTextBlock;
class UPBInventoryViewModel;
class FDragDropEvent;
struct FPBInventorySlotData;

// 인벤토리 그리드의 단일 슬롯 위젯
UCLASS()
class PROJECTB3_API UPBInventorySlotWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯 인덱스와 ViewModel을 바인딩
	void InitializeSlot(int32 InSlotIndex, UPBInventoryViewModel* InViewModel);

	// 현재 슬롯 데이터를 기반으로 UI를 갱신
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void RefreshSlot();

protected:
	/*~ UUserWidget Interface ~*/
	// 버튼 이벤트를 바인딩
	virtual void NativeConstruct() override;

	// 버튼 이벤트를 해제
	virtual void NativeDestruct() override;

	// 우클릭 장착 동작을 처리
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 더블클릭 사용/장착 동작을 처리
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 드래그 시작 시 Payload를 구성
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	// 다른 슬롯에서 드래그된 아이템을 드롭했을 때 이동을 처리
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// 마우스 포인터 진입 시 툴팁 표시
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스 포인터 이탈 시 툴팁 숨김
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

public:
	// 우클릭 시 브로드캐스트 — PBInventoryPanelWidget이 구독하여 컨텍스트 메뉴를 표시
	FOnSlotContextMenuRequestedSignature OnSlotContextMenuRequested;

private:
	// 좌클릭 선택 동작을 처리
	UFUNCTION()
	void OnSlotClicked();

	// 우클릭 컨텍스트 메뉴 요청 동작을 처리
	void OnSlotRightClicked(FVector2D ScreenPosition);

	// 더블클릭 자동 장착 동작을 처리
	void OnSlotDoubleClicked();

	// 현재 슬롯의 유효한 아이템 데이터를 조회
	bool GetCurrentSlotData(FPBInventorySlotData& OutSlotData) const;

protected:
	// 표시할 툴팁 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Tooltip")
	TSubclassOf<class UPBItemTooltipWidget> TooltipWidgetClass;

private:
	// 생성되어 캐싱된 툴팁 위젯
	UPROPERTY(Transient)
	TObjectPtr<class UPBItemTooltipWidget> CachedTooltipWidget;

	// 아이템 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	// 스택 수량 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StackCountText;

	// 등급 테두리
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RarityBorder;

	// 슬롯 클릭 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

	// 바인딩된 슬롯 인덱스
	int32 SlotIndex = INDEX_NONE;

	// 인벤토리 ViewModel 참조
	TWeakObjectPtr<UPBInventoryViewModel> InventoryViewModel;
};
