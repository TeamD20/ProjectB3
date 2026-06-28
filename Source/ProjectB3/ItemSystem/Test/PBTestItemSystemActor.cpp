// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (아이템 시스템 테스트 액터)

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
