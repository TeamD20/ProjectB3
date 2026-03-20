// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBActionIndicatorViewModel.h"

void UPBActionIndicatorViewModel::SetAction(const FPBActionIndicatorData& NewAction)
{
	CurrentAction = NewAction;
	CurrentAction.bIsActive = true;
	OnActionChanged.Broadcast(CurrentAction);
}

void UPBActionIndicatorViewModel::ClearAction()
{
	if (!CurrentAction.bIsActive)
	{
		return;
	}

	CurrentAction.ActionType = EPBActionIndicatorType::None;
	CurrentAction.DisplayText = FText::GetEmpty();
	CurrentAction.Icon = nullptr;
	CurrentAction.bIsActive = false;
	
	OnActionChanged.Broadcast(CurrentAction);
}
