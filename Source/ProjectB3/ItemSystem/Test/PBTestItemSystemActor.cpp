// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTestItemSystemActor.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"

APBTestItemSystemActor::APBTestItemSystemActor()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("ASC"));
	InventoryComponent = CreateDefaultSubobject<UPBInventoryComponent>(TEXT("Inventory"));
	EquipmentComponent = CreateDefaultSubobject<UPBEquipmentComponent>(TEXT("Equipment"));
}

UAbilitySystemComponent* APBTestItemSystemActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
