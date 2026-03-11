// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnIndicatorWidget.h"
#include "PBTurnPortraitViewModel.h"
#include "Components/Border.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"

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

void UPBTurnIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 스스로 ViewModel을 찾아 바인딩
	if (!TurnOrderViewModel)
	{
		UPBTurnOrderViewModel* TurnVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBTurnOrderViewModel>(GetOwningLocalPlayer());
		if (TurnVM)
		{
			SetupViewModel(TurnVM);
		}
	}

	if (TurnIndicatorBorder)
	{
		TurnIndicatorBorder->SetVisibility(ESlateVisibility::Hidden);
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
	if (TurnIndicatorBorder)
	{
		TurnIndicatorBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	
	if (NewTurnOwner)
	{
		BP_PlayTurnIndicatorAnimation(NewTurnOwner->GetDisplayName(), NewTurnOwner->IsAlly());
	}
}
