// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCombatLogEntryWidget.h"

#include "Components/TextBlock.h"

void UPBCombatLogEntryWidget::SetEntryData(const FPBCombatLogEntry& Entry)
{
	if (!IsValid(EntryText))
	{
		return;
	}

	EntryText->SetText(Entry.LogText);
	EntryText->SetColorAndOpacity(FSlateColor(Entry.TextColor));
}
