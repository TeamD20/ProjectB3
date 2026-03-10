// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnPortraitWidget.h"
#include "PBTurnPortraitViewModel.h"

void UPBTurnPortraitWidget::SetPortraitViewModel(UPBTurnPortraitViewModel* InViewModel)
{
	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.RemoveAll(this);
		PortraitViewModel->OnDeathStateChanged.RemoveAll(this);
		PortraitViewModel->OnDisplayNameChanged.RemoveAll(this);
		PortraitViewModel->OnPortraitChanged.RemoveAll(this);
	}

	PortraitViewModel = InViewModel;

	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleCurrentTurnChanged);
		PortraitViewModel->OnDeathStateChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleDeathStateChanged);
		PortraitViewModel->OnDisplayNameChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleDisplayNameChanged);
		PortraitViewModel->OnPortraitChanged.AddUObject(this, &UPBTurnPortraitWidget::HandlePortraitChanged);

		// 초기 동기화
		HandleDisplayNameChanged(PortraitViewModel->GetDisplayName());
		HandlePortraitChanged(PortraitViewModel->GetPortrait());
		BP_OnInitAllyState(PortraitViewModel->IsAlly());
		HandleCurrentTurnChanged(PortraitViewModel->IsCurrentTurn());
		HandleDeathStateChanged(PortraitViewModel->IsDead());
	}
}

void UPBTurnPortraitWidget::NativeDestruct()
{
	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.RemoveAll(this);
		PortraitViewModel->OnDeathStateChanged.RemoveAll(this);
		PortraitViewModel->OnDisplayNameChanged.RemoveAll(this);
		PortraitViewModel->OnPortraitChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

#include "ProjectB3/UI/Common/PBPortraitBaseWidget.h"

void UPBTurnPortraitWidget::HandleCurrentTurnChanged(bool bIsCurrentTurn)
{
	BP_OnCurrentTurnChanged(bIsCurrentTurn);
}

void UPBTurnPortraitWidget::HandleDeathStateChanged(bool bIsDead)
{
	BP_OnDeathStateChanged(bIsDead);
}

void UPBTurnPortraitWidget::HandleDisplayNameChanged(FText NewName)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetDisplayName(NewName);
	}
}

void UPBTurnPortraitWidget::HandlePortraitChanged(TSoftObjectPtr<UTexture2D> NewPortrait)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetPortraitImage(NewPortrait);
	}
}
