// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInteraction_PickupAction.h"

#include "GameFramework/Pawn.h"
#include "ProjectB3/ItemSystem/PBFieldDropActor.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"

UPBInteraction_PickupAction::UPBInteraction_PickupAction()
{
	InteractionType = EPBInteractionType::Instant;
	bRequiresRange = false;
	Priority = 0;
}

bool UPBInteraction_PickupAction::CanInteract_Implementation(AActor* Interactor) const
{
	APawn* Pawn = GetPawn(Interactor);
	if (!IsValid(Pawn))
	{
		return false;
	}

	// FieldDropActor가 유효한 아이템 인스턴스를 보유 중인지 확인
	const APBFieldDropActor* FieldDrop = Cast<APBFieldDropActor>(GetOwner());
	if (!IsValid(FieldDrop) || !FieldDrop->GetDroppedItem().IsValid())
	{
		return false;
	}

	// Interactor의 인벤토리에 빈 슬롯이 있는지 확인
	const UPBInventoryComponent* Inventory = Pawn->FindComponentByClass<UPBInventoryComponent>();
	if (!IsValid(Inventory))
	{
		return false;
	}

	return Inventory->HasFreeSlot();
}

void UPBInteraction_PickupAction::Execute_Implementation(AActor* Interactor)
{
	APawn* Pawn = GetPawn(Interactor);
	if (!IsValid(Pawn))
	{
		return;
	}

	APBFieldDropActor* FieldDrop = Cast<APBFieldDropActor>(GetOwner());
	if (!IsValid(FieldDrop))
	{
		return;
	}

	UPBInventoryComponent* Inventory = Pawn->FindComponentByClass<UPBInventoryComponent>();
	if (!IsValid(Inventory))
	{
		return;
	}

	// 아이템 인스턴스를 Interactor 인벤토리로 이전
	const bool bAdded = Inventory->AddItemInstance(FieldDrop->GetDroppedItem());
	if (bAdded)
	{
		// 줍기 성공 시 필드 드롭 액터를 월드에서 제거
		FieldDrop->Destroy();
	}
}
