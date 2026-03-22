// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBFieldDropActor.h"

#include "Components/StaticMeshComponent.h"
#include "ProjectB3/Interaction/PBInteractableComponent.h"
#include "ProjectB3/Interaction/Actions/PBInteraction_PickupAction.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"

APBFieldDropActor::APBFieldDropActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);

	InteractableComponent = CreateDefaultSubobject<UPBInteractableComponent>(TEXT("InteractableComponent"));
	UPBInteraction_PickupAction* PickupAction = CreateDefaultSubobject<UPBInteraction_PickupAction>(TEXT("PickupAction"));
	InteractableComponent->InteractionActions.Add(PickupAction);
}

void APBFieldDropActor::BeginPlay()
{
	Super::BeginPlay();
}

void APBFieldDropActor::InitializeDrop(const FPBItemInstance& InItemInstance)
{
	DroppedItem = InItemInstance;

	// DataAsset에 드롭 메시가 지정되어 있으면 적용
	if (IsValid(ItemMesh) && IsValid(DroppedItem.ItemDataAsset))
	{
		if (UStaticMesh* DropMesh = DroppedItem.ItemDataAsset->DropMesh)
		{
			ItemMesh->SetStaticMesh(DropMesh);
		}
	}
}
