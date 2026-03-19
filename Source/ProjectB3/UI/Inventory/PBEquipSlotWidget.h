// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBEquipSlotWidget.generated.h"

class UBorder;
class UButton;
class UDragDropOperation;
class UImage;
class UPBInventoryViewModel;
class FDragDropEvent;

// 장비창 개별 슬롯 위젯
UCLASS()
class PROJECTB3_API UPBEquipSlotWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯에 ViewModel을 바인딩
	void InitializeSlot(UPBInventoryViewModel* InViewModel);

	// 슬롯 데이터를 기반으로 UI를 갱신
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void RefreshSlot();

	// 무기 세트 활성 하이라이트를 설정
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void SetHighlighted(bool bInHighlighted);

	// 바인딩된 장비 슬롯 키를 반환
	UFUNCTION(BlueprintPure, Category = "UI|Inventory")
	EPBEquipSlot GetBoundSlot() const { return BoundSlot; }

protected:
	/*~ UUserWidget Interface ~*/
	// 버튼 이벤트를 바인딩
	virtual void NativeConstruct() override;

	// 버튼 이벤트를 해제
	virtual void NativeDestruct() override;

	// 인벤토리 슬롯 드롭 시 BoundSlot 장착을 처리
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	// 마우스 툴팁 표시
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 마우스 툴팁 숨김
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	// 슬롯 클릭 시 장비 해제 동작을 수행
	UFUNCTION()
	void OnSlotClicked();

protected:
	// 이 위젯이 표현하는 장비 슬롯 키
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "UI|Inventory")
	EPBEquipSlot BoundSlot = EPBEquipSlot::Head;

	// 표시할 툴팁 위젯 클래스 (블루프린트에서 WBP_ItemTooltip 할당)
	UPROPERTY(EditDefaultsOnly, Category = "UI|Tooltip")
	TSubclassOf<class UPBItemTooltipWidget> TooltipWidgetClass;

private:
	// 장비 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EquipmentIcon;

	// 빈 슬롯 기본 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EmptySlotIcon;

	// 활성 무기 세트 하이라이트 테두리
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> HighlightBorder;

	// 슬롯 클릭 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

	// 하이라이트 상태
	bool bIsHighlighted = false;

	// 인벤토리 ViewModel 참조
	TWeakObjectPtr<UPBInventoryViewModel> InventoryViewModel;

	// 생성되어 캐싱된 툴팁 위젯
	UPROPERTY(Transient)
	TObjectPtr<class UPBItemTooltipWidget> CachedTooltipWidget;
};
