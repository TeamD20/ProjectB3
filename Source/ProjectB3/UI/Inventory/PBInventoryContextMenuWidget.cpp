// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInventoryContextMenuWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "ProjectB3/AbilitySystem/Payload/PBConsumableUsePayload.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Data/PBConsumableDataAsset.h"
#include "ProjectB3/PBGameplayTags.h"

void UPBInventoryContextMenuWidget::Initialize(
	const FGuid& InInstanceID,
	EPBItemType InItemType,
	UPBInventoryComponent* InInventory,
	UPBEquipmentComponent* InEquipment,
	AActor* InTargetActor,
	FVector2D ScreenPosition)
{
	InstanceID = InInstanceID;
	InventoryComponent = InInventory;
	EquipmentComponent = InEquipment;
	TargetActor = InTargetActor;

	// ContentPanel을 커서 위치로 이동 — 위젯 자체는 풀스크린으로 유지되어 BackdropButton이 화면 전체를 덮음
	// ScreenPosition은 뷰포트 기준 물리 픽셀 좌표이므로 DPI 스케일로 나눠 Slate 단위로 변환
	if (IsValid(ContentPanel))
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ContentPanel->Slot))
		{
			const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);
			CanvasSlot->SetPosition(DPIScale > 0.f ? ScreenPosition / DPIScale : ScreenPosition);
		}
	}

	// 아이템 타입에 따라 버튼 가시성 결정
	const bool bIsEquipment = (InItemType == EPBItemType::Weapon
		|| InItemType == EPBItemType::Armor
		|| InItemType == EPBItemType::Trinket);
	const bool bIsConsumable = (InItemType == EPBItemType::Consumable);

	if (IsValid(EquipButton))
	{
		EquipButton->SetVisibility(bIsEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (IsValid(UseButton))
	{
		UseButton->SetVisibility(bIsConsumable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPBInventoryContextMenuWidget::CloseMenu()
{
	SetVisibility(ESlateVisibility::Collapsed);

	InstanceID = FGuid();
	InventoryComponent = nullptr;
	EquipmentComponent = nullptr;
	TargetActor = nullptr;
}

void UPBInventoryContextMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(BackdropButton))
	{
		BackdropButton->OnClicked.AddDynamic(this, &ThisClass::HandleBackdropClicked);
	}
	if (IsValid(EquipButton))
	{
		EquipButton->OnClicked.AddDynamic(this, &ThisClass::HandleEquipClicked);
	}
	if (IsValid(UseButton))
	{
		UseButton->OnClicked.AddDynamic(this, &ThisClass::HandleUseClicked);
	}
	if (IsValid(DiscardButton))
	{
		DiscardButton->OnClicked.AddDynamic(this, &ThisClass::HandleDiscardClicked);
	}
}

void UPBInventoryContextMenuWidget::NativeDestruct()
{
	if (IsValid(BackdropButton))
	{
		BackdropButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleBackdropClicked);
	}
	if (IsValid(EquipButton))
	{
		EquipButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleEquipClicked);
	}
	if (IsValid(UseButton))
	{
		UseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleUseClicked);
	}
	if (IsValid(DiscardButton))
	{
		DiscardButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleDiscardClicked);
	}

	Super::NativeDestruct();
}

void UPBInventoryContextMenuWidget::HandleEquipClicked()
{
	if (IsValid(EquipmentComponent) && IsValid(InventoryComponent) && InstanceID.IsValid())
	{
		EquipmentComponent->AutoEquipItem(InstanceID, InventoryComponent);
	}
	CloseMenu();
}

void UPBInventoryContextMenuWidget::HandleUseClicked()
{
	if (!IsValid(InventoryComponent) || !IsValid(TargetActor) || !InstanceID.IsValid())
	{
		CloseMenu();
		return;
	}

	const FPBItemInstance ItemInstance = InventoryComponent->FindItemByID(InstanceID);
	const UPBConsumableDataAsset* ConsumableDA = Cast<UPBConsumableDataAsset>(ItemInstance.ItemDataAsset);
	if (!IsValid(ConsumableDA))
	{
		CloseMenu();
		return;
	}

	// UseConsumable 어빌리티 트리거 — 스택 차감은 어빌리티 콜백이 처리
	UPBConsumableUsePayload* Payload = NewObject<UPBConsumableUsePayload>();
	Payload->InstanceID = InstanceID;
	Payload->ConsumableDataAsset = const_cast<UPBConsumableDataAsset*>(ConsumableDA);

	FGameplayEventData EventData;
	EventData.OptionalObject = Payload;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		TargetActor,
		PBGameplayTags::Event_Item_UseConsumable,
		EventData);

	CloseMenu();
}

void UPBInventoryContextMenuWidget::HandleDiscardClicked()
{
	if (IsValid(InventoryComponent) && InstanceID.IsValid())
	{
		InventoryComponent->RemoveItem(InstanceID, 1);
	}
	CloseMenu();
}

void UPBInventoryContextMenuWidget::HandleBackdropClicked()
{
	CloseMenu();
}
