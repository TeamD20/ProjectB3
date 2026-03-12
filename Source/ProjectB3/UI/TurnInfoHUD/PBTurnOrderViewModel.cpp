// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnOrderViewModel.h"

void UPBTurnOrderViewModel::SetTurnOrder(const TArray<FPBTurnOrderEntry>& InData)
{
	PortraitViewModels.Empty();
	CurrentTurnIndex = -1;

	for (const FPBTurnOrderEntry& Entry : InData)
	{
		UPBTurnPortraitViewModel* NewViewModel = NewObject<UPBTurnPortraitViewModel>(this);
		NewViewModel->InitializeTurnPortrait(Entry);
		PortraitViewModels.Add(NewViewModel);
	}

	OnTurnOrderListChanged.Broadcast();
}

void UPBTurnOrderViewModel::SetTurnIndex(int32 NewTurnIndex)
{
	if (PortraitViewModels.Num() == 0 || CurrentTurnIndex == NewTurnIndex)
	{
		return;
	}

	if (PortraitViewModels.IsValidIndex(CurrentTurnIndex))
	{
		PortraitViewModels[CurrentTurnIndex]->SetIsCurrentTurn(false);
	}
	
	if (PortraitViewModels.IsValidIndex(NewTurnIndex))
	{
		CurrentTurnIndex = NewTurnIndex;
		UPBTurnPortraitViewModel* NewTurnViewModel = PortraitViewModels[NewTurnIndex];
	
		if (NewTurnViewModel)
		{
			NewTurnViewModel->SetIsCurrentTurn(true);
			OnTurnAdvanced.Broadcast(NewTurnViewModel);
		}	
	}
}

void UPBTurnOrderViewModel::AdvanceTurn()
{
	if (PortraitViewModels.Num() == 0) return;

	if (PortraitViewModels.IsValidIndex(CurrentTurnIndex))
	{
		PortraitViewModels[CurrentTurnIndex]->SetIsCurrentTurn(false);
	}

	CurrentTurnIndex = (CurrentTurnIndex + 1) % PortraitViewModels.Num();

	UPBTurnPortraitViewModel* CurrentViewModel = PortraitViewModels[CurrentTurnIndex];
	
	if (CurrentViewModel)
	{
		CurrentViewModel->SetIsCurrentTurn(true);
		OnTurnAdvanced.Broadcast(CurrentViewModel);
	}
}

void UPBTurnOrderViewModel::UpdatePortraitState(int32 TargetIndex, bool bIsDead)
{
	if (PortraitViewModels.IsValidIndex(TargetIndex))
	{
		PortraitViewModels[TargetIndex]->SetIsDead(bIsDead);
	}
}
