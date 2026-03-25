// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBSkillBarTypes.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBEquipmentSlotWidget.generated.h"

class UImage;
class UButton;
class UTextBlock;
class UPBInventoryViewModel;
class UPBItemTooltipWidget;

/** 범용적으로 재사용 가능한 장비(무기/방어구 등) 슬롯 위젯 */
UCLASS(Abstract)
class PROJECTB3_API UPBEquipmentSlotWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯에 표시될 장비 데이터를 설정합니다.
	UFUNCTION(BlueprintCallable, Category = "UI|EquipmentSlot")
	void UpdateSlot(const FPBEquipmentSlotData& InData);

	// 인벤토리 ViewModel을 바인딩하여 장비 변경 이벤트 구독 및 툴팁 기능을 활성화합니다.
	UFUNCTION(BlueprintCallable, Category = "UI|EquipmentSlot")
	void InitializeWithInventory(UPBInventoryViewModel* InViewModel);

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	// 슬롯 클릭 시 호출되는 내부 이벤트
	UFUNCTION()
	virtual void OnSlotClicked();

	// 블루프린트에서 슬롯 클릭 시 추가 연출/기능을 구현할 수 있도록 제공
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|EquipmentSlot")
	void BP_OnSlotClicked();

private:
	// 장비 변경 이벤트 핸들러
	void HandleEquipmentSlotUpdated(EPBEquipSlot InSlot);

	// ViewModel 데이터로 아이콘 동기화
	void RefreshFromViewModel();

protected:
	// 이 위젯이 표현하는 장비 슬롯 키 (블루프린트 인스턴스에서 설정)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "UI|EquipmentSlot")
	EPBEquipSlot BoundSlot = EPBEquipSlot::WeaponSet1_Main;

	// 표시할 툴팁 위젯 클래스 (블루프린트에서 WBP_ItemTooltip 할당)
	UPROPERTY(EditDefaultsOnly, Category = "UI|Tooltip")
	TSubclassOf<UPBItemTooltipWidget> TooltipWidgetClass;

	// 장비 이미지를 표시할 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EquipmentIcon;

	// 수량 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText;

	// 슬롯 상호작용 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

private:
	// 인벤토리 ViewModel 참조
	TWeakObjectPtr<UPBInventoryViewModel> InventoryViewModel;

	// 생성되어 캐싱된 툴팁 위젯
	UPROPERTY(Transient)
	TObjectPtr<UPBItemTooltipWidget> CachedTooltipWidget;
};
