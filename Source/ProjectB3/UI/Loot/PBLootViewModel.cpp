// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBLootViewModel.h"

#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"

void UPBLootViewModel::InitializeForActor(AActor* InTargetActor, ULocalPlayer* InLocalPlayer)
{
	Super::InitializeForActor(InTargetActor, InLocalPlayer);

	TargetInventory = nullptr;
	TargetEquipment = nullptr;
	LootSlots.Reset();
	OwnerName = FText::GetEmpty();

	if (!IsValid(InTargetActor))
	{
		return;
	}

	// 대상 액터에서 인벤토리/장비 컴포넌트 획득
	TargetInventory = InTargetActor->FindComponentByClass<UPBInventoryComponent>();
	TargetEquipment = InTargetActor->FindComponentByClass<UPBEquipmentComponent>();

	if (IsValid(TargetInventory))
	{
		TargetInventory->OnInventoryItemChanged.AddDynamic(this, &ThisClass::HandleTargetInventoryChanged);
		TargetInventory->OnInventoryFullRefresh.AddDynamic(this, &ThisClass::HandleTargetInventoryFullRefresh);
	}

	if (IsValid(TargetEquipment))
	{
		TargetEquipment->OnEquipmentSlotChanged.AddDynamic(this, &ThisClass::HandleTargetEquipmentChanged);
	}

	// 대상 이름 설정 (CombatDisplayName 우선, 없으면 액터 오브젝트명 폴백)
	if (const APBCharacterBase* Character = Cast<APBCharacterBase>(InTargetActor))
	{
		OwnerName = Character->GetCombatDisplayName();
	}
	if (OwnerName.IsEmpty())
	{
		OwnerName = FText::FromString(InTargetActor->GetName());
	}

	BuildLootSlots();
}

void UPBLootViewModel::Deinitialize()
{
	if (IsValid(TargetInventory))
	{
		TargetInventory->OnInventoryItemChanged.RemoveDynamic(this, &ThisClass::HandleTargetInventoryChanged);
		TargetInventory->OnInventoryFullRefresh.RemoveDynamic(this, &ThisClass::HandleTargetInventoryFullRefresh);
	}

	if (IsValid(TargetEquipment))
	{
		TargetEquipment->OnEquipmentSlotChanged.RemoveDynamic(this, &ThisClass::HandleTargetEquipmentChanged);
	}

	TargetInventory = nullptr;
	TargetEquipment = nullptr;
	PlayerInventory = nullptr;
	LootSlots.Reset();
	OwnerName = FText::GetEmpty();

	Super::Deinitialize();
}

void UPBLootViewModel::SetPlayerInventory(UPBInventoryComponent* InPlayerInventory)
{
	PlayerInventory = InPlayerInventory;
}

bool UPBLootViewModel::TakeItem(int32 SlotIndex)
{
	if (!LootSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	if (!IsValid(TargetInventory) || !IsValid(PlayerInventory))
	{
		return false;
	}

	const FPBLootSlotData& LootSlot = LootSlots[SlotIndex];
	const FGuid InstanceID = LootSlot.SlotData.InstanceID;

	// 장비 해제 → 인벤토리 이동 → 플레이어 이동 과정에서 중간 갱신 억제
	bIsTakingItem = true;

	// 장착 중인 장비는 먼저 해제하여 대상 인벤토리로 이동
	if (LootSlot.bIsEquipped && IsValid(TargetEquipment))
	{
		if (!TargetEquipment->UnequipItem(LootSlot.EquipSlot, TargetInventory))
		{
			bIsTakingItem = false;
			return false;
		}
	}

	// 대상 인벤토리 → 플레이어 인벤토리로 이동
	const bool bSuccess = UPBInventoryComponent::TransferItem(InstanceID, TargetInventory, PlayerInventory);

	bIsTakingItem = false;

	// 모든 작업 완료 후 한 번만 갱신
	BuildLootSlots();
	OnLootSlotsRefreshed.Broadcast();

	return bSuccess;
}

void UPBLootViewModel::TakeAllItems()
{
	if (!IsValid(TargetInventory) || !IsValid(PlayerInventory))
	{
		return;
	}

	// 인덱스 변동을 피하기 위해 역순으로 순회
	for (int32 i = LootSlots.Num() - 1; i >= 0; --i)
	{
		// 플레이어 인벤토리 공간이 없으면 중단
		if (!PlayerInventory->HasFreeSlot())
		{
			break;
		}

		TakeItem(i);
	}
}

void UPBLootViewModel::BuildLootSlots()
{
	LootSlots.Reset();

	// 1. 장착 중인 장비 슬롯 추가 (장비 컴포넌트가 있을 때만)
	if (IsValid(TargetEquipment))
	{
		for (int32 SlotValue = 0; SlotValue < static_cast<int32>(EPBEquipSlot::MAX); ++SlotValue)
		{
			const EPBEquipSlot Slot = static_cast<EPBEquipSlot>(SlotValue);
			const FPBItemInstance EquippedItem = TargetEquipment->GetEquippedItem(Slot);

			if (!EquippedItem.IsValid())
			{
				continue;
			}

			FPBLootSlotData LootSlot;
			LootSlot.SlotData = BuildSlotData(EquippedItem);
			LootSlot.bIsEquipped = true;
			LootSlot.EquipSlot = Slot;
			LootSlots.Add(LootSlot);
		}
	}

	// 2. 인벤토리 아이템 추가
	if (IsValid(TargetInventory))
	{
		const TArray<FPBItemInstance>& Items = TargetInventory->GetItems();
		for (const FPBItemInstance& Item : Items)
		{
			if (!Item.IsValid())
			{
				continue;
			}

			FPBLootSlotData LootSlot;
			LootSlot.SlotData = BuildSlotData(Item);
			LootSlot.bIsEquipped = false;
			LootSlots.Add(LootSlot);
		}
	}
}

FPBInventorySlotData UPBLootViewModel::BuildSlotData(const FPBItemInstance& ItemInstance) const
{
	FPBInventorySlotData Result;

	if (!ItemInstance.IsValid() || !IsValid(ItemInstance.ItemDataAsset))
	{
		return Result;
	}

	Result.bIsEmpty = false;
	Result.ItemIcon = ItemInstance.ItemDataAsset->ItemIcon;
	Result.ItemName = ItemInstance.ItemDataAsset->ItemName;
	Result.StackCount = ItemInstance.Count;
	Result.RarityColor = GetRarityColor(ItemInstance.ItemDataAsset->Rarity);
	Result.InstanceID = ItemInstance.InstanceID;
	return Result;
}

FLinearColor UPBLootViewModel::GetRarityColor(EPBItemRarity Rarity) const
{
	switch (Rarity)
	{
	case EPBItemRarity::Common:     return FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
	case EPBItemRarity::Uncommon:   return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
	case EPBItemRarity::Rare:       return FLinearColor(0.3f, 0.5f, 1.0f, 1.0f);
	case EPBItemRarity::Legendary:  return FLinearColor(1.0f, 0.7f, 0.1f, 1.0f);
	default:                        return FLinearColor::White;
	}
}

void UPBLootViewModel::HandleTargetInventoryChanged(int32 /*SlotIndex*/)
{
	// TakeItem 중에는 중간 갱신을 억제하고, 완료 후 한 번만 갱신
	if (bIsTakingItem)
	{
		return;
	}

	// 단일 슬롯 변경도 인덱스 재매핑이 필요하므로 전체 재구성
	BuildLootSlots();
	OnLootSlotsRefreshed.Broadcast();
}

void UPBLootViewModel::HandleTargetInventoryFullRefresh()
{
	if (bIsTakingItem)
	{
		return;
	}

	BuildLootSlots();
	OnLootSlotsRefreshed.Broadcast();
}

void UPBLootViewModel::HandleTargetEquipmentChanged(EPBEquipSlot /*Slot*/)
{
	if (bIsTakingItem)
	{
		return;
	}

	BuildLootSlots();
	OnLootSlotsRefreshed.Broadcast();
}
