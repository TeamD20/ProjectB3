// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnIndicatorWidget.h"
#include "PBTurnPortraitViewModel.h"

void UPBTurnIndicatorWidget::SetupViewModel(UPBTurnOrderViewModel* InViewModel)
{
	if (TurnOrderViewModel)
	{
		TurnOrderViewModel->OnTurnAdvanced.RemoveAll(this);
	}

	TurnOrderViewModel = InViewModel;

	if (TurnOrderViewModel)
	{
		TurnOrderViewModel->OnTurnAdvanced.AddUObject(this, &UPBTurnIndicatorWidget::HandleTurnAdvanced);
	}
}

void UPBTurnIndicatorWidget::NativeDestruct()
{
	if (TurnOrderViewModel)
	{
		TurnOrderViewModel->OnTurnAdvanced.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UPBTurnIndicatorWidget::HandleTurnAdvanced(UPBTurnPortraitViewModel* NewTurnOwner)
{
	if (NewTurnOwner)
	{
		BP_PlayTurnIndicatorAnimation(NewTurnOwner->GetDisplayName(), NewTurnOwner->IsAlly());
	}
}
