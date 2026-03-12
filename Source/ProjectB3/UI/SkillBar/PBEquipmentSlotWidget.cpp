// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEquipmentSlotWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"

void UPBEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &UPBEquipmentSlotWidget::OnSlotClicked);
	}
}

void UPBEquipmentSlotWidget::SetEquipmentIcon(UTexture2D* InIcon)
{
	if (EquipmentIcon)
	{
		if (InIcon)
		{
			EquipmentIcon->SetBrushFromTexture(InIcon);
			EquipmentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			// 아이콘이 없으면 숨김 처리 (배경 슬롯만 보이게 됨)
			EquipmentIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UPBEquipmentSlotWidget::OnSlotClicked()
{
	// C++ 로직 (향후 무기 스왑이나 툴팁 팝업 등 확장 가능)
	
	// 블루프린트로 이벤트 전달 (선택 연출 등)
	BP_OnSlotClicked();
}
