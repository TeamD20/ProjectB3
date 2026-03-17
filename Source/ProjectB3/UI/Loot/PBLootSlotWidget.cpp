// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBLootSlotWidget.h"
#include "PBLootViewModel.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UPBLootSlotWidget::InitializeSlot(int32 InSlotIndex, UPBLootViewModel* InViewModel)
{
	SlotIndex = InSlotIndex;
	LootViewModel = InViewModel;
	RefreshSlot();
}

void UPBLootSlotWidget::RefreshSlot()
{
	if (!LootViewModel.IsValid() || !LootViewModel->GetLootSlots().IsValidIndex(SlotIndex))
	{
		// 빈 슬롯 처리
		if (IsValid(ItemIcon))
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
		if (IsValid(StackCountText))
		{
			StackCountText->SetVisibility(ESlateVisibility::Hidden);
		}
		if (IsValid(EquippedBadge))
		{
			EquippedBadge->SetVisibility(ESlateVisibility::Hidden);
		}
		return;
	}

	const FPBLootSlotData& LootSlot = LootViewModel->GetLootSlots()[SlotIndex];
	const FPBInventorySlotData& Data = LootSlot.SlotData;

	// 아이콘
	if (IsValid(ItemIcon))
	{
		if (!Data.bIsEmpty && !Data.ItemIcon.IsNull())
		{
			ItemIcon->SetBrushFromSoftTexture(Data.ItemIcon);
			ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 수량 텍스트 (2 이상일 때만 표시)
	if (IsValid(StackCountText))
	{
		if (Data.StackCount > 1)
		{
			StackCountText->SetText(FText::AsNumber(Data.StackCount));
			StackCountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			StackCountText->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 등급 테두리 색상
	if (IsValid(RarityBorder))
	{
		RarityBorder->SetBrushColor(Data.RarityColor);
	}

	// 장착 중 뱃지 (장비 아이템만 표시)
	if (IsValid(EquippedBadge))
	{
		EquippedBadge->SetVisibility(
			LootSlot.bIsEquipped ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UPBLootSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.AddDynamic(this, &ThisClass::OnSlotClicked);
	}
}

void UPBLootSlotWidget::NativeDestruct()
{
	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.RemoveDynamic(this, &ThisClass::OnSlotClicked);
	}

	Super::NativeDestruct();
}

void UPBLootSlotWidget::OnSlotClicked()
{
	if (!LootViewModel.IsValid())
	{
		return;
	}

	LootViewModel->TakeItem(SlotIndex);
}
