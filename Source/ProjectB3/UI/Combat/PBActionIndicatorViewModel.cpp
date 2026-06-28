// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 강리한 (전투 행동 인디케이터 상태 연결)

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
