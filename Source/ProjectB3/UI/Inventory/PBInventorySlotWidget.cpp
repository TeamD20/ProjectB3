// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInventorySlotWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Input/Events.h"
#include "Input/Reply.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/UI/Inventory/PBInventoryDragDropOperation.h"
#include "ProjectB3/UI/Inventory/PBInventorySlotData.h"
#include "ProjectB3/UI/Inventory/PBInventoryViewModel.h"

namespace
{
	// 드롭/장착 경로에서 반복되는 인벤토리 컴포넌트 조회를 공통화
	bool ResolveInventoryComponents(
		AActor* SourceActor,
		AActor* TargetActor,
		UPBInventoryComponent*& OutSourceInventory,
		UPBInventoryComponent*& OutTargetInventory)
	{
		if (!IsValid(SourceActor) || !IsValid(TargetActor))
		{
			return false;
		}

		OutSourceInventory = SourceActor->FindComponentByClass<UPBInventoryComponent>();
		OutTargetInventory = TargetActor->FindComponentByClass<UPBInventoryComponent>();
		return IsValid(OutSourceInventory) && IsValid(OutTargetInventory);
	}
}

void UPBInventorySlotWidget::InitializeSlot(int32 InSlotIndex, UPBInventoryViewModel* InViewModel)
{
	SlotIndex = InSlotIndex;
	InventoryViewModel = InViewModel;
	RefreshSlot();
}

void UPBInventorySlotWidget::RefreshSlot()
{
	if (!InventoryViewModel.IsValid())
	{
		return;
	}

	FPBInventorySlotData SlotData;
	if (!InventoryViewModel->GetInventorySlotData(SlotIndex, SlotData))
	{
		return;
	}

	// ViewModel 슬롯 스냅샷을 위젯의 시각 상태로 반영
	if (IsValid(ItemIcon))
	{
		if (!SlotData.bIsEmpty && !SlotData.ItemIcon.IsNull())
		{
			ItemIcon->SetBrushFromSoftTexture(SlotData.ItemIcon);
			ItemIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (IsValid(StackCountText))
	{
		if (!SlotData.bIsEmpty && SlotData.StackCount > 1)
		{
			StackCountText->SetText(FText::AsNumber(SlotData.StackCount));
			StackCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			StackCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (IsValid(RarityBorder))
	{
		RarityBorder->SetBrushColor(SlotData.bIsEmpty ? FLinearColor::Transparent : SlotData.RarityColor);
	}

	if (IsValid(SlotButton))
	{
		SlotButton->SetIsEnabled(!SlotData.bIsEmpty);
	}
}

void UPBInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.AddDynamic(this, &ThisClass::OnSlotClicked);
	}
}

void UPBInventorySlotWidget::NativeDestruct()
{
	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.RemoveDynamic(this, &ThisClass::OnSlotClicked);
	}

	InventoryViewModel.Reset();
	SlotIndex = INDEX_NONE;

	Super::NativeDestruct();
}

FReply UPBInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnSlotRightClicked();
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		FPBInventorySlotData SlotData;
		if (GetCurrentSlotData(SlotData))
		{
			// 유효한 아이템 슬롯에서만 드래그를 시작
			return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UPBInventorySlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnSlotRightClicked();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UPBInventorySlotWidget::OnSlotClicked()
{
	// 현재는 선택 상태 저장 없이 즉시 시각 동기화만 수행
	RefreshSlot();
}

void UPBInventorySlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	FPBInventorySlotData SlotData;
	if (!GetCurrentSlotData(SlotData))
	{
		return;
	}

	AActor* SourceActor = InventoryViewModel.IsValid() ? InventoryViewModel->GetTargetActor() : nullptr;
	if (!IsValid(SourceActor))
	{
		return;
	}

	UPBInventoryDragDropOperation* NewOperation = NewObject<UPBInventoryDragDropOperation>(this);
	if (!IsValid(NewOperation))
	{
		return;
	}

	NewOperation->Payload = this;
	NewOperation->SourceActor = SourceActor;
	NewOperation->SourceSlotIndex = SlotIndex;
	NewOperation->InstanceID = SlotData.InstanceID;
	OutOperation = NewOperation;
}

bool UPBInventorySlotWidget::NativeOnDrop(
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

	if (DragOperation->SourceSlotIndex == SlotIndex
		&& DragOperation->SourceActor == (InventoryViewModel.IsValid() ? InventoryViewModel->GetTargetActor() : nullptr))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	AActor* SourceActor = DragOperation->SourceActor.Get();
	AActor* TargetActor = InventoryViewModel->GetTargetActor();

	UPBInventoryComponent* SourceInventory = nullptr;
	UPBInventoryComponent* TargetInventory = nullptr;
	if (!ResolveInventoryComponents(SourceActor, TargetActor, SourceInventory, TargetInventory))
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	if (SourceActor == TargetActor)
	{
		FPBInventorySlotData TargetSlotData;
		if (!InventoryViewModel->GetInventorySlotData(SlotIndex, TargetSlotData))
		{
			return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
		}

		if (TargetSlotData.bIsEmpty)
		{
			// 동일 인벤토리의 빈 슬롯 드롭은 대상 인덱스 기준 재배치로 처리
			if (UPBInventoryComponent::MoveItemToSlot(DragOperation->InstanceID, SlotIndex, TargetInventory))
			{
				return true;
			}

			return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
		}

		if (UPBInventoryComponent::SwapItems(DragOperation->InstanceID, TargetSlotData.InstanceID, TargetInventory))
		{
			return true;
		}

		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	// 파티원 간 드래그 앤 드롭 이동은 TransferItem 헬퍼로 통일
	if (UPBInventoryComponent::TransferItem(DragOperation->InstanceID, SourceInventory, TargetInventory))
	{
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UPBInventorySlotWidget::OnSlotRightClicked()
{
	if (!InventoryViewModel.IsValid())
	{
		return;
	}

	FPBInventorySlotData SlotData;
	if (!InventoryViewModel->GetInventorySlotData(SlotIndex, SlotData) || SlotData.bIsEmpty)
	{
		return;
	}

	AActor* TargetActor = InventoryViewModel->GetTargetActor();
	if (!IsValid(TargetActor))
	{
		return;
	}

	UPBInventoryComponent* InventoryComponent = nullptr;
	UPBInventoryComponent* DummyInventory = nullptr;
	if (!ResolveInventoryComponents(TargetActor, TargetActor, InventoryComponent, DummyInventory))
	{
		return;
	}

	UPBEquipmentComponent* EquipmentComponent = TargetActor->FindComponentByClass<UPBEquipmentComponent>();
	if (!IsValid(InventoryComponent) || !IsValid(EquipmentComponent))
	{
		return;
	}

	// 우클릭/더블클릭은 동일한 자동 장착 경로로 통일
	EquipmentComponent->AutoEquipItem(SlotData.InstanceID, InventoryComponent);
}

bool UPBInventorySlotWidget::GetCurrentSlotData(FPBInventorySlotData& OutSlotData) const
{
	if (!InventoryViewModel.IsValid())
	{
		return false;
	}

	if (!InventoryViewModel->GetInventorySlotData(SlotIndex, OutSlotData))
	{
		return false;
	}

	return !OutSlotData.bIsEmpty && OutSlotData.InstanceID.IsValid();
}
