// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEquipmentSlotWidget.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "ProjectB3/UI/Inventory/PBInventoryViewModel.h"
#include "ProjectB3/UI/Inventory/PBItemTooltipWidget.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"

void UPBEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &UPBEquipmentSlotWidget::OnSlotClicked);
	}
}

void UPBEquipmentSlotWidget::NativeDestruct()
{
	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPBEquipmentSlotWidget::OnSlotClicked);
	}

	if (InventoryViewModel.IsValid())
	{
		InventoryViewModel->OnEquipmentSlotUpdated.RemoveAll(this);
	}
	InventoryViewModel.Reset();

	if (IsValid(CachedTooltipWidget))
	{
		CachedTooltipWidget->RemoveFromParent();
		CachedTooltipWidget = nullptr;
	}

	Super::NativeDestruct();
}

void UPBEquipmentSlotWidget::InitializeWithInventory(UPBInventoryViewModel* InViewModel)
{
	// 기존 바인딩 해제
	if (InventoryViewModel.IsValid())
	{
		InventoryViewModel->OnEquipmentSlotUpdated.RemoveAll(this);
	}

	SetToolTip(nullptr);
	if (IsValid(CachedTooltipWidget))
	{
		CachedTooltipWidget->RemoveFromParent();
		CachedTooltipWidget = nullptr;
	}

	InventoryViewModel = InViewModel;

	if (InventoryViewModel.IsValid())
	{
		InventoryViewModel->OnEquipmentSlotUpdated.AddUObject(this, &UPBEquipmentSlotWidget::HandleEquipmentSlotUpdated);
		RefreshFromViewModel();
	}
}

void UPBEquipmentSlotWidget::HandleEquipmentSlotUpdated(EPBEquipSlot InSlot)
{
	// 이 위젯이 담당하는 슬롯이 변경되었을 때만 갱신
	if (InSlot == BoundSlot)
	{
		RefreshFromViewModel();
	}
}

void UPBEquipmentSlotWidget::RefreshFromViewModel()
{
	if (!InventoryViewModel.IsValid())
	{
		return;
	}

	FPBInventorySlotData SlotData;
	if (!InventoryViewModel->GetEquipmentSlotData(BoundSlot, SlotData))
	{
		return;
	}

	if (EquipmentIcon)
	{
		if (!SlotData.bIsEmpty && !SlotData.ItemIcon.IsNull())
		{
			EquipmentIcon->SetBrushFromSoftTexture(SlotData.ItemIcon);
			EquipmentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			EquipmentIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (QuantityText)
	{
		QuantityText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UPBEquipmentSlotWidget::UpdateSlot(const FPBEquipmentSlotData& InData)
{
	// ViewModel이 바인딩되어 있으면 ViewModel 데이터 우선 사용
	if (InventoryViewModel.IsValid())
	{
		RefreshFromViewModel();
		return;
	}

	// ViewModel이 없을 때는 기존 FPBEquipmentSlotData 기반 갱신 (폴백)
	if (EquipmentIcon)
	{
		if (!InData.Icon.IsNull())
		{
			EquipmentIcon->SetBrushFromSoftTexture(InData.Icon);
			EquipmentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			EquipmentIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (QuantityText)
	{
		if (InData.Quantity > 0)
		{
			QuantityText->SetText(FText::AsNumber(InData.Quantity));
			QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			QuantityText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UPBEquipmentSlotWidget::OnSlotClicked()
{
	BP_OnSlotClicked();
}

void UPBEquipmentSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!InventoryViewModel.IsValid() || !TooltipWidgetClass)
	{
		return;
	}

	// 장비 슬롯이 비어 있으면 툴팁 불필요
	FPBInventorySlotData SlotData;
	if (!InventoryViewModel->GetEquipmentSlotData(BoundSlot, SlotData) || SlotData.bIsEmpty)
	{
		return;
	}

	AActor* TargetActor = InventoryViewModel->GetTargetActor();
	if (!IsValid(TargetActor))
	{
		return;
	}

	// 장비된 아이템은 인벤토리에서 빠져있으므로 EquipmentComponent에서 직접 조회
	UPBEquipmentComponent* EquipmentComponent = TargetActor->FindComponentByClass<UPBEquipmentComponent>();
	if (!IsValid(EquipmentComponent))
	{
		return;
	}

	FPBItemInstance ItemInstance = EquipmentComponent->GetEquippedItem(BoundSlot);
	if (ItemInstance.IsValid())
	{
		if (!IsValid(CachedTooltipWidget))
		{
			CachedTooltipWidget = CreateWidget<UPBItemTooltipWidget>(this, TooltipWidgetClass);
			if (CachedTooltipWidget)
			{
				CachedTooltipWidget->BindViewModel(InventoryViewModel.Get());
			}
		}

		if (IsValid(CachedTooltipWidget) && InventoryViewModel.IsValid())
		{
			InventoryViewModel->RequestTooltipData(ItemInstance.InstanceID);
			SetToolTip(CachedTooltipWidget);
		}
	}
}

void UPBEquipmentSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	SetToolTip(nullptr);
}
