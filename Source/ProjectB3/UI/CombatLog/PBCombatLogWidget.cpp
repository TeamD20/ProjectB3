// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCombatLogWidget.h"

#include "Components/ScrollBox.h"
#include "PBCombatLogEntryWidget.h"
#include "PBCombatLogViewModel.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"

void UPBCombatLogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CombatLogVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBCombatLogViewModel>(GetOwningLocalPlayer());
	if (!IsValid(CombatLogVM))
	{
		return;
	}

	// 델리게이트 바인딩
	EntryAddedHandle = CombatLogVM->OnEntryAdded.AddUObject(this, &ThisClass::AddEntryWidget);
	LogClearedHandle = CombatLogVM->OnLogCleared.AddUObject(this, &ThisClass::ClearEntryWidgets);

	// 위젯이 늦게 생성된 경우 기존 로그를 복원
	RebuildAllEntries();
}

void UPBCombatLogWidget::NativeDestruct()
{
	if (IsValid(CombatLogVM))
	{
		if (EntryAddedHandle.IsValid())
		{
			CombatLogVM->OnEntryAdded.Remove(EntryAddedHandle);
			EntryAddedHandle.Reset();
		}

		if (LogClearedHandle.IsValid())
		{
			CombatLogVM->OnLogCleared.Remove(LogClearedHandle);
			LogClearedHandle.Reset();
		}
	}

	CombatLogVM = nullptr;

	Super::NativeDestruct();
}

void UPBCombatLogWidget::AddEntryWidget(const FPBCombatLogEntry& Entry)
{
	if (!IsValid(ScrollBox) || !EntryWidgetClass)
	{
		return;
	}

	// ScrollBox 최대 자식 수 초과 시 가장 오래된 항목 제거
	constexpr int32 MaxWidgetCount = 200;
	while (ScrollBox->GetChildrenCount() >= MaxWidgetCount)
	{
		ScrollBox->RemoveChildAt(0);
	}

	UPBCombatLogEntryWidget* EntryWidget = CreateWidget<UPBCombatLogEntryWidget>(this, EntryWidgetClass);
	if (!IsValid(EntryWidget))
	{
		return;
	}

	EntryWidget->SetEntryData(Entry);
	ScrollBox->AddChild(EntryWidget);

	if (bAutoScrollEnabled)
	{
		ScrollBox->ScrollToEnd();
	}
}

void UPBCombatLogWidget::ClearEntryWidgets()
{
	if (IsValid(ScrollBox))
	{
		ScrollBox->ClearChildren();
	}
}

void UPBCombatLogWidget::RebuildAllEntries()
{
	if (!IsValid(CombatLogVM) || !IsValid(ScrollBox))
	{
		return;
	}

	ClearEntryWidgets();

	for (const FPBCombatLogEntry& Entry : CombatLogVM->GetAllEntries())
	{
		AddEntryWidget(Entry);
	}
}
