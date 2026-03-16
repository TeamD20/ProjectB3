// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBResourceGroupWidget.h"
#include "PBResourceSlotWidget.h"
#include "Components/PanelWidget.h"

void UPBResourceGroupWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		// 에디터 뷰포트에서 프리뷰용으로 미리보기용 슬롯 생성
		RebuildSlots(DefaultMaxCount);
		SetResourceCount(DefaultMaxCount, DefaultMaxCount); // 기본적으로 가득 찬 모습으로 프리뷰
	}
}

void UPBResourceGroupWidget::SetResourceCount(int32 CurrentCount, int32 MaxCount)
{
	// 슬롯 개수가 변경되었다면 재구성
	if (CreatedSlots.Num() != MaxCount)
	{
		RebuildSlots(MaxCount);
	}

	// 각 슬롯의 활성/비활성 상태 갱신 (왼쪽부터 CurrentCount만큼 채움)
	for (int32 i = 0; i < CreatedSlots.Num(); ++i)
	{
		if (CreatedSlots[i])
		{
			CreatedSlots[i]->SetIsActive(i < CurrentCount);
		}
	}
}

void UPBResourceGroupWidget::RebuildSlots(int32 MaxCount)
{
	if (!SlotContainer || !SlotWidgetClass)
	{
		return;
	}

	SlotContainer->ClearChildren();
	CreatedSlots.Empty();

	for (int32 i = 0; i < MaxCount; ++i)
	{
		UPBResourceSlotWidget* NewSlot = CreateWidget<UPBResourceSlotWidget>(this, SlotWidgetClass);
		if (NewSlot)
		{
			NewSlot->SetTextures(BackgroundTexture, ActiveTexture);
			SlotContainer->AddChild(NewSlot);
			CreatedSlots.Add(NewSlot);
		}
	}
}
