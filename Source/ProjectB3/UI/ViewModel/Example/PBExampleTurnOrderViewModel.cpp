// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBExampleTurnOrderViewModel.h"
#include "ProjectB3/UI/PBUITags.h"

UPBExampleTurnOrderViewModel::UPBExampleTurnOrderViewModel()
{
	ViewModelTag = PBUITags::UI_ViewModel_TurnOrder;
}

void UPBExampleTurnOrderViewModel::SetTurnOrder(const TArray<FPBTurnOrderEntry>& InOrder)
{
	TurnOrder = InOrder;
	CurrentTurnIndex = 0;
	
	OnTurnOrderChanged.Broadcast();
}

void UPBExampleTurnOrderViewModel::AdvanceTurn()
{
	if (TurnOrder.IsEmpty())
	{
		return;
	}

	CurrentTurnIndex++;
	
	if (CurrentTurnIndex >= TurnOrder.Num())
	{
		CurrentTurnIndex = 0;
		SetRoundNumber(RoundNumber + 1);
	}

	OnTurnAdvanced.Broadcast(CurrentTurnIndex);
}

void UPBExampleTurnOrderViewModel::SetRoundNumber(int32 InRound)
{
	if (RoundNumber != InRound)
	{
		RoundNumber = InRound;
	}
}

FPBTurnOrderEntry UPBExampleTurnOrderViewModel::GetCurrentEntry() const
{
	if (TurnOrder.IsValidIndex(CurrentTurnIndex))
	{
		return TurnOrder[CurrentTurnIndex];
	}
	return FPBTurnOrderEntry();
}

FText UPBExampleTurnOrderViewModel::GetRoundText() const
{
	return FText::Format(FText::FromString(TEXT("Round {0}")), FText::AsNumber(RoundNumber));
}

int32 UPBExampleTurnOrderViewModel::GetRemainingTurnsInRound() const
{
	if (TurnOrder.IsEmpty())
	{
		return 0;
	}
	
	return FMath::Max(0, TurnOrder.Num() - CurrentTurnIndex - 1);
}
