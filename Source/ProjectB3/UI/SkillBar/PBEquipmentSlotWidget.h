// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBSkillBarTypes.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBEquipmentSlotWidget.generated.h"

class UImage;
class UButton;
class UTextBlock;

/** 범용적으로 재사용 가능한 장비(무기/방어구 등) 슬롯 위젯 */
UCLASS(Abstract)
class PROJECTB3_API UPBEquipmentSlotWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯에 표시될 장비 데이터를 설정합니다.
	UFUNCTION(BlueprintCallable, Category = "UI|EquipmentSlot")
	void UpdateSlot(const FPBEquipmentSlotData& InData);

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;

	// 슬롯 클릭 시 호출되는 내부 이벤트
	UFUNCTION()
	virtual void OnSlotClicked();

	// 블루프린트에서 슬롯 클릭 시 추가 연출/기능을 구현할 수 있도록 제공
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|EquipmentSlot")
	void BP_OnSlotClicked();

protected:
	// 장비 이미지를 표시할 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EquipmentIcon;

	// 수량 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText;

	// 슬롯 상호작용 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;
};
