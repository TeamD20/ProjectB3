// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBInventory, Log, All);

UPBInventoryComponent::UPBInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UPBInventoryComponent::AddItem(UPBItemDataAsset* ItemData, int32 Amount)
{
	if (!IsValid(ItemData) || Amount <= 0)
	{
		UE_LOG(LogPBInventory, Warning, TEXT("AddItem: 유효하지 않은 아이템 또는 수량 (%d)"), Amount);
		return 0;
	}

	int32 TotalAdded = 0;

	// 스택 가능한 아이템이면 기존 슬롯에 합산 시도
	if (ItemData->IsStackable())
	{
		int32 StackSlot = FindStackableSlotIndex(ItemData);
		if (StackSlot != INDEX_NONE)
		{
			FPBItemInstance& Existing = Items[StackSlot];
			const int32 SpaceLeft = ItemData->MaxStackSize - Existing.Count;
			const int32 ToAdd = FMath::Min(Amount, SpaceLeft);
			Existing.Count += ToAdd;
			TotalAdded += ToAdd;
			Amount -= ToAdd;

			OnInventoryItemChanged.Broadcast(StackSlot);

			UE_LOG(LogPBInventory, Verbose, TEXT("AddItem: 기존 스택에 %d개 합산 (슬롯 %d)"), ToAdd, StackSlot);
		}
	}

	// 남은 수량을 새 슬롯에 배치
	while (Amount > 0 && HasFreeSlot())
	{
		const int32 ToAdd = FMath::Min(Amount, ItemData->MaxStackSize);
		FPBItemInstance NewInstance = FPBItemInstance::Create(ItemData, ToAdd);
		const int32 NewIndex = Items.Add(NewInstance);
		TotalAdded += ToAdd;
		Amount -= ToAdd;

		OnInventoryItemChanged.Broadcast(NewIndex);

		UE_LOG(LogPBInventory, Verbose, TEXT("AddItem: 새 슬롯 %d에 %d개 추가 (%s)"),
			NewIndex, ToAdd, *ItemData->GetName());
	}

	if (Amount > 0)
	{
		UE_LOG(LogPBInventory, Warning, TEXT("AddItem: 인벤토리 부족으로 %d개 미추가"), Amount);
	}

	return TotalAdded;
}

bool UPBInventoryComponent::AddItemInstance(const FPBItemInstance& ItemInstance)
{
	if (!ItemInstance.IsValid())
	{
		UE_LOG(LogPBInventory, Warning, TEXT("AddItemInstance: 유효하지 않은 아이템 인스턴스"));
		return false;
	}

	if (!HasFreeSlot())
	{
		UE_LOG(LogPBInventory, Warning, TEXT("AddItemInstance: 인벤토리 공간 부족"));
		return false;
	}

	const int32 NewIndex = Items.Add(ItemInstance);
	OnInventoryItemChanged.Broadcast(NewIndex);
	return true;
}

bool UPBInventoryComponent::RemoveItem(const FGuid& InstanceID, int32 Amount)
{
	const int32 SlotIndex = FindSlotIndexByID(InstanceID);
	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogPBInventory, Warning, TEXT("RemoveItem: InstanceID를 찾을 수 없음"));
		return false;
	}

	if (Amount <= 0)
	{
		return false;
	}

	FPBItemInstance& Instance = Items[SlotIndex];
	if (Amount >= Instance.Count)
	{
		// 슬롯 완전 제거
		Items.RemoveAt(SlotIndex);
		// 제거된 이후 슬롯들의 인덱스가 변경되므로 전체 갱신
		OnInventoryFullRefresh.Broadcast();
	}
	else
	{
		Instance.Count -= Amount;
		OnInventoryItemChanged.Broadcast(SlotIndex);
	}

	return true;
}

bool UPBInventoryComponent::TransferItem(const FGuid& InstanceID, UPBInventoryComponent* Source, UPBInventoryComponent* Target)
{
	if (!IsValid(Source) || !IsValid(Target) || Source == Target)
	{
		return false;
	}

	const FPBItemInstance SourceItem = Source->FindItemByID(InstanceID);
	if (!SourceItem.IsValid())
	{
		UE_LOG(LogPBInventory, Warning, TEXT("TransferItem: Source에서 아이템을 찾을 수 없음"));
		return false;
	}

	if (!Target->AddItemInstance(SourceItem))
	{
		UE_LOG(LogPBInventory, Warning, TEXT("TransferItem: Target 추가 실패"));
		return false;
	}

	if (!Source->RemoveItem(InstanceID, SourceItem.Count))
	{
		UE_LOG(LogPBInventory, Warning, TEXT("TransferItem: Source 제거 실패, 롤백 수행"));
		Target->RemoveItem(SourceItem.InstanceID, SourceItem.Count);
		return false;
	}

	return true;
}

bool UPBInventoryComponent::SwapItems(const FGuid& FirstInstanceID, const FGuid& SecondInstanceID, UPBInventoryComponent* Inventory)
{
	if (!IsValid(Inventory) || !FirstInstanceID.IsValid() || !SecondInstanceID.IsValid() || FirstInstanceID == SecondInstanceID)
	{
		return false;
	}

	const int32 FirstIndex = Inventory->FindSlotIndexByID(FirstInstanceID);
	const int32 SecondIndex = Inventory->FindSlotIndexByID(SecondInstanceID);
	if (!Inventory->Items.IsValidIndex(FirstIndex) || !Inventory->Items.IsValidIndex(SecondIndex))
	{
		return false;
	}

	if (FirstIndex == SecondIndex)
	{
		return false;
	}

	Inventory->Items.Swap(FirstIndex, SecondIndex);
	Inventory->OnInventoryItemChanged.Broadcast(FirstIndex);
	Inventory->OnInventoryItemChanged.Broadcast(SecondIndex);
	return true;
}

bool UPBInventoryComponent::MoveItemToSlot(const FGuid& InstanceID, int32 TargetSlotIndex, UPBInventoryComponent* Inventory)
{
	if (!IsValid(Inventory) || !InstanceID.IsValid() || TargetSlotIndex < 0)
	{
		return false;
	}

	const int32 SourceIndex = Inventory->FindSlotIndexByID(InstanceID);
	if (!Inventory->Items.IsValidIndex(SourceIndex))
	{
		return false;
	}

	if (Inventory->Items.Num() <= 1)
	{
		return true;
	}

	// 빈 슬롯 인덱스로 드롭된 경우 마지막 사용 슬롯으로 클램프한다.
	int32 ClampedTargetIndex = FMath::Clamp(TargetSlotIndex, 0, Inventory->Items.Num() - 1);
	if (SourceIndex == ClampedTargetIndex)
	{
		return true;
	}

	FPBItemInstance MovedItem = Inventory->Items[SourceIndex];
	Inventory->Items.RemoveAt(SourceIndex);

	// Source를 먼저 제거했기 때문에 뒤쪽으로 이동할 때는 목표 인덱스를 한 칸 보정한다.
	if (SourceIndex < ClampedTargetIndex)
	{
		--ClampedTargetIndex;
	}

	Inventory->Items.Insert(MovedItem, ClampedTargetIndex);

	// 다수 인덱스가 변동되므로 전체 리프레시 이벤트를 사용한다.
	Inventory->OnInventoryFullRefresh.Broadcast();
	return true;
}

FPBItemInstance UPBInventoryComponent::GetItemAtSlot(int32 SlotIndex) const
{
	if (Items.IsValidIndex(SlotIndex))
	{
		return Items[SlotIndex];
	}

	return FPBItemInstance();
}

FPBItemInstance UPBInventoryComponent::FindItemByID(const FGuid& InstanceID) const
{
	for (const FPBItemInstance& Item : Items)
	{
		if (Item.InstanceID == InstanceID)
		{
			return Item;
		}
	}

	return FPBItemInstance();
}

int32 UPBInventoryComponent::FindSlotIndexByID(const FGuid& InstanceID) const
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].InstanceID == InstanceID)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

int32 UPBInventoryComponent::FindStackableSlotIndex(const UPBItemDataAsset* ItemData) const
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].ItemDataAsset == ItemData && Items[i].Count < ItemData->MaxStackSize)
		{
			return i;
		}
	}

	return INDEX_NONE;
}
