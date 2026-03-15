// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEquipSlotWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/UI/Inventory/PBInventoryDragDropOperation.h"
#include "ProjectB3/UI/Inventory/PBInventoryViewModel.h"

namespace
{
	// 장비 슬롯 상호작용에서 반복되는 컴포넌트 조회를 공통화
	bool ResolveTargetComponents(
		AActor* TargetActor,
		UPBInventoryComponent*& OutInventory,
		UPBEquipmentComponent*& OutEquipment)
	{
		if (!IsValid(TargetActor))
		{
			return false;
		}

		OutInventory = TargetActor->FindComponentByClass<UPBInventoryComponent>();
		OutEquipment = TargetActor->FindComponentByClass<UPBEquipmentComponent>();
		return IsValid(OutInventory) && IsValid(OutEquipment);
	}
}

void UPBEquipSlotWidget::InitializeSlot(UPBInventoryViewModel* InViewModel)
{
	InventoryViewModel = InViewModel;
	RefreshSlot();
}

void UPBEquipSlotWidget::RefreshSlot()
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

	// 장비 유무에 따라 실제 아이콘/빈 슬롯 아이콘 가시성을 전환
	if (IsValid(EquipmentIcon))
	{
		if (!SlotData.bIsEmpty && !SlotData.ItemIcon.IsNull())
		{
			EquipmentIcon->SetBrushFromSoftTexture(SlotData.ItemIcon);
			EquipmentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			EquipmentIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (IsValid(EmptySlotIcon))
	{
		EmptySlotIcon->SetVisibility(SlotData.bIsEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	SetHighlighted(bIsHighlighted);
}

void UPBEquipSlotWidget::SetHighlighted(bool bInHighlighted)
{
	bIsHighlighted = bInHighlighted;

	if (IsValid(HighlightBorder))
	{
		HighlightBorder->SetVisibility(bIsHighlighted ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPBEquipSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.AddDynamic(this, &ThisClass::OnSlotClicked);
	}
}

void UPBEquipSlotWidget::NativeDestruct()
{
	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.RemoveDynamic(this, &ThisClass::OnSlotClicked);
	}

	InventoryViewModel.Reset();

	Super::NativeDestruct();
}

bool UPBEquipSlotWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (!IsValid(InOperation) || !InventoryViewModel.IsValid())
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UPBInventoryDragDropOperation* DragOperation = Cast<UPBInventoryDragDropOperation>(InOperation);
	if (!IsValid(DragOperation) || !DragOperation->InstanceID.IsValid())
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	AActor* TargetActor = InventoryViewModel->GetTargetActor();
	AActor* SourceActor = DragOperation->SourceActor.Get();

	if (!IsValid(TargetActor))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UPBInventoryComponent* TargetInventory = nullptr;
	UPBEquipmentComponent* TargetEquipment = nullptr;
	if (!ResolveTargetComponents(TargetActor, TargetInventory, TargetEquipment))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	// SourceActor를 찾지 못하면 동일 액터 장착으로 처리하지 않고 종료
	if (!IsValid(SourceActor))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UPBInventoryComponent* SourceInventory = SourceActor->FindComponentByClass<UPBInventoryComponent>();
	if (!IsValid(SourceInventory))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	if (TargetEquipment->EquipItemFromExternalInventory(DragOperation->InstanceID, BoundSlot, SourceInventory, TargetInventory))
	{
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UPBEquipSlotWidget::OnSlotClicked()
{
	if (!InventoryViewModel.IsValid())
	{
		return;
	}

	AActor* TargetActor = InventoryViewModel->GetTargetActor();
	if (!IsValid(TargetActor))
	{
		return;
	}

	UPBInventoryComponent* InventoryComponent = nullptr;
	UPBEquipmentComponent* EquipmentComponent = nullptr;
	if (!ResolveTargetComponents(TargetActor, InventoryComponent, EquipmentComponent))
	{
		return;
	}

	// 장비 슬롯 클릭은 즉시 해제 요청으로 처리
	EquipmentComponent->UnequipItem(BoundSlot, InventoryComponent);
}
