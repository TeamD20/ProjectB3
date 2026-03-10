// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnOrderInfoWidget.h"
#include "PBTurnOrderViewModel.h"
#include "PBTurnPortraitViewModel.h"
#include "PBTurnPortraitWidget.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UPBTurnOrderInfoWidget::SetupViewModel(UPBTurnOrderViewModel* InViewModel)
{
	if (TurnOrderViewModel)
	{
		TurnOrderViewModel->OnTurnOrderListChanged.RemoveAll(this);
	}

	TurnOrderViewModel = InViewModel;

	if (TurnOrderViewModel)
	{
		TurnOrderViewModel->OnTurnOrderListChanged.AddUObject(this, &UPBTurnOrderInfoWidget::HandleTurnOrderListChanged);
		
		// 초기 리스트 동기화
		HandleTurnOrderListChanged();
	}
}

void UPBTurnOrderInfoWidget::NativeDestruct()
{
	if (TurnOrderViewModel)
	{
		TurnOrderViewModel->OnTurnOrderListChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UPBTurnOrderInfoWidget::HandleTurnOrderListChanged()
{
	if (!TurnPortraitContainer || !PortraitWidgetClass || !TurnOrderViewModel)
	{
		return;
	}

	TurnPortraitContainer->ClearChildren();

	const TArray<UPBTurnPortraitViewModel*>& ViewModels = TurnOrderViewModel->GetPortraitViewModels();

	for (UPBTurnPortraitViewModel* VM : ViewModels)
	{
		UPBTurnPortraitWidget* NewWidget = CreateWidget<UPBTurnPortraitWidget>(this, PortraitWidgetClass);
		if (NewWidget)
		{
			NewWidget->SetPortraitViewModel(VM);
			TurnPortraitContainer->AddChildToHorizontalBox(NewWidget);
		}
	}
}
