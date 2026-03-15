// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBItemSystemLibrary.h"
#include "Components/PBInventoryComponent.h"
#include "Components/PBEquipmentComponent.h"
#include "Data/PBEquipmentDataAsset.h"

UPBInventoryComponent* UPBItemSystemLibrary::GetInventoryComponent(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	return Actor->FindComponentByClass<UPBInventoryComponent>();
}

UPBEquipmentComponent* UPBItemSystemLibrary::GetEquipmentComponent(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	return Actor->FindComponentByClass<UPBEquipmentComponent>();
}

bool UPBItemSystemLibrary::EquipItemToSlot(AActor* Actor, const FGuid& InstanceID, EPBEquipSlot Slot)
{
	UPBEquipmentComponent* EquipComp = GetEquipmentComponent(Actor);
	UPBInventoryComponent* InvComp = GetInventoryComponent(Actor);

	if (!IsValid(EquipComp) || !IsValid(InvComp))
	{
		return false;
	}

	return EquipComp->EquipItem(InstanceID, Slot, InvComp);
}

bool UPBItemSystemLibrary::AutoEquipItem(AActor* Actor, const FGuid& InstanceID)
{
	UPBEquipmentComponent* EquipComp = GetEquipmentComponent(Actor);
	UPBInventoryComponent* InvComp = GetInventoryComponent(Actor);

	if (!IsValid(EquipComp) || !IsValid(InvComp))
	{
		return false;
	}

	return EquipComp->AutoEquipItem(InstanceID, InvComp);
}

bool UPBItemSystemLibrary::UnequipSlot(AActor* Actor, EPBEquipSlot Slot)
{
	UPBEquipmentComponent* EquipComp = GetEquipmentComponent(Actor);
	UPBInventoryComponent* InvComp = GetInventoryComponent(Actor);

	if (!IsValid(EquipComp) || !IsValid(InvComp))
	{
		return false;
	}

	return EquipComp->UnequipItem(Slot, InvComp);
}

FPBItemInstance UPBItemSystemLibrary::GetEquippedItem(const AActor* Actor, EPBEquipSlot Slot)
{
	if (const UPBEquipmentComponent* EquipComp = GetEquipmentComponent(Actor))
	{
		return EquipComp->GetEquippedItem(Slot);
	}

	return FPBItemInstance();
}

bool UPBItemSystemLibrary::IsSlotEmpty(const AActor* Actor, EPBEquipSlot Slot)
{
	if (const UPBEquipmentComponent* EquipComp = GetEquipmentComponent(Actor))
	{
		return EquipComp->IsSlotEmpty(Slot);
	}

	return true;
}

bool UPBItemSystemLibrary::IsEquippableItem(const UPBItemDataAsset* ItemData)
{
	return IsValid(Cast<UPBEquipmentDataAsset>(ItemData));
}
