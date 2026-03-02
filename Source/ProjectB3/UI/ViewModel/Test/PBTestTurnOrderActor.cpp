// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTestTurnOrderActor.h"
#include "ProjectB3/UI/ViewModel/Example/PBExampleTurnOrderViewModel.h"
#include "ProjectB3/UI/ViewModel/Example/PBExampleTurnOrderWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"

APBTestTurnOrderActor::APBTestTurnOrderActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APBTestTurnOrderActor::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	// 1. ViewModel 생성 + 더미 데이터 주입
	UPBViewModelBase* VMBase = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel(PC, UPBExampleTurnOrderViewModel::StaticClass());

	if (UPBExampleTurnOrderViewModel* TurnOrderVM = Cast<UPBExampleTurnOrderViewModel>(VMBase))
	{
		TArray<FPBTurnOrderEntry> DummyEntries;

		FPBTurnOrderEntry Entry1;
		Entry1.DisplayName = FText::FromString(TEXT("Shadowheart"));
		Entry1.Initiative = 18;
		Entry1.bIsAlly = true;
		DummyEntries.Add(Entry1);

		FPBTurnOrderEntry Entry2;
		Entry2.DisplayName = FText::FromString(TEXT("Lae'zel"));
		Entry2.Initiative = 15;
		Entry2.bIsAlly = true;
		DummyEntries.Add(Entry2);

		FPBTurnOrderEntry Entry3;
		Entry3.DisplayName = FText::FromString(TEXT("Goblin Warrior"));
		Entry3.Initiative = 12;
		Entry3.bIsAlly = false;
		DummyEntries.Add(Entry3);

		FPBTurnOrderEntry Entry4;
		Entry4.DisplayName = FText::FromString(TEXT("Skeleton Archer"));
		Entry4.Initiative = 8;
		Entry4.bIsAlly = false;
		DummyEntries.Add(Entry4);

		TurnOrderVM->SetTurnOrder(DummyEntries);
		TurnOrderVM->SetRoundNumber(1);
		TurnOrderVM->SetDesiredVisibility(true);
	}

	// 2. Widget PushUI
	UPBUIBlueprintLibrary::PushUI(PC, UPBExampleTurnOrderWidget::StaticClass());
}

void APBTestTurnOrderActor::TestAdvanceTurn()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UPBViewModelBase* VMBase = UPBUIBlueprintLibrary::GetGlobalViewModel(PC, UPBExampleTurnOrderViewModel::StaticClass());
		if (UPBExampleTurnOrderViewModel* TurnOrderVM = Cast<UPBExampleTurnOrderViewModel>(VMBase))
		{
			TurnOrderVM->AdvanceTurn();
		}
	}
}

void APBTestTurnOrderActor::TestShuffleTurnOrder()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UPBViewModelBase* VMBase = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel(PC, UPBExampleTurnOrderViewModel::StaticClass());
		if (UPBExampleTurnOrderViewModel* TurnOrderVM = Cast<UPBExampleTurnOrderViewModel>(VMBase))
		{
			TArray<FPBTurnOrderEntry> DummyEntries;
			
			FPBTurnOrderEntry Entry3;
			Entry3.DisplayName = FText::FromString(TEXT("Goblin Warrior"));
			Entry3.Initiative = 19;
			Entry3.bIsAlly = false;
			DummyEntries.Add(Entry3);

			FPBTurnOrderEntry Entry1;
			Entry1.DisplayName = FText::FromString(TEXT("Shadowheart"));
			Entry1.Initiative = 14;
			Entry1.bIsAlly = true;
			DummyEntries.Add(Entry1);

			FPBTurnOrderEntry Entry2;
			Entry2.DisplayName = FText::FromString(TEXT("Lae'zel"));
			Entry2.Initiative = 9;
			Entry2.bIsAlly = true;
			DummyEntries.Add(Entry2);
			
			FPBTurnOrderEntry Entry4;
			Entry4.DisplayName = FText::FromString(TEXT("Skeleton Archer"));
			Entry4.Initiative = 4;
			Entry4.bIsAlly = false;
			DummyEntries.Add(Entry4);

			TurnOrderVM->SetTurnOrder(DummyEntries);
			TurnOrderVM->SetRoundNumber(TurnOrderVM->GetRoundText().IsEmpty() ? 1 : 1);
		}
	}
}

void APBTestTurnOrderActor::TestClearViewModel()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UPBViewModelBase* VMBase = UPBUIBlueprintLibrary::GetGlobalViewModel(PC, UPBExampleTurnOrderViewModel::StaticClass());
		if (UPBExampleTurnOrderViewModel* TurnOrderVM = Cast<UPBExampleTurnOrderViewModel>(VMBase))
		{
			TurnOrderVM->SetTurnOrder(TArray<FPBTurnOrderEntry>());
			TurnOrderVM->SetDesiredVisibility(false);
		}
	}
}
