// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBLootPanelWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "PBLootSlotWidget.h"
#include "PBLootViewModel.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"

void UPBLootPanelWidget::InitializeLoot(AActor* TargetActor, APlayerController* InPlayerController)
{
	if (!IsValid(TargetActor) || !IsValid(InPlayerController))
	{
		return;
	}

	CachedPlayerController = InPlayerController;

	// ViewModelSubsystem에서 LootViewModel 생성
	// GetOrCreateActorViewModel 내부에서 InitializeForActor가 호출되므로 별도 호출하지 않는다.
	UPBViewModelSubsystem* ViewModelSubsystem = UPBUIBlueprintLibrary::GetViewModelSubsystem(InPlayerController);
	if (!IsValid(ViewModelSubsystem))
	{
		return;
	}

	LootViewModel = ViewModelSubsystem->GetOrCreateActorViewModel<UPBLootViewModel>(TargetActor);
	if (!IsValid(LootViewModel))
	{
		return;
	}

	// 플레이어 인벤토리 설정
	if (APawn* Pawn = InPlayerController->GetPawn())
	{
		UPBInventoryComponent* PlayerInventory = Pawn->FindComponentByClass<UPBInventoryComponent>();
		LootViewModel->SetPlayerInventory(PlayerInventory);
	}

	// 슬롯 갱신 이벤트 구독
	LootSlotsRefreshedHandle = LootViewModel->OnLootSlotsRefreshed.AddUObject(
		this, &ThisClass::HandleLootSlotsRefreshed);

	// 대상 이름 표시
	if (IsValid(OwnerNameText))
	{
		OwnerNameText->SetText(LootViewModel->GetOwnerName());
	}

	RebuildSlotWidgets();
}

void UPBLootPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::OnCloseClicked);
	}

	if (IsValid(TakeAllButton))
	{
		TakeAllButton->OnClicked.AddDynamic(this, &ThisClass::OnTakeAllClicked);
	}
}

void UPBLootPanelWidget::NativeDestruct()
{
	if (IsValid(CloseButton))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::OnCloseClicked);
	}

	if (IsValid(TakeAllButton))
	{
		TakeAllButton->OnClicked.RemoveDynamic(this, &ThisClass::OnTakeAllClicked);
	}

	if (IsValid(LootViewModel) && LootSlotsRefreshedHandle.IsValid())
	{
		LootViewModel->OnLootSlotsRefreshed.Remove(LootSlotsRefreshedHandle);
	}

	// 서브시스템을 통해 ViewModel을 정리 (Deinitialize + 맵에서 제거)
	if (IsValid(LootViewModel) && CachedPlayerController.IsValid())
	{
		UPBViewModelSubsystem* ViewModelSubsystem = UPBUIBlueprintLibrary::GetViewModelSubsystem(CachedPlayerController.Get());
		if (IsValid(ViewModelSubsystem))
		{
			AActor* TargetActor = LootViewModel->GetTargetActor();
			if (IsValid(TargetActor))
			{
				ViewModelSubsystem->RemoveActorViewModel(TargetActor, UPBLootViewModel::StaticClass());
			}
		}
	}

	LootViewModel = nullptr;
	SlotWidgets.Reset();

	Super::NativeDestruct();
}

void UPBLootPanelWidget::RebuildSlotWidgets()
{
	if (!IsValid(LootSlotsPanel) || !LootSlotWidgetClass || !IsValid(LootViewModel))
	{
		return;
	}

	// 기존 슬롯 위젯 제거
	LootSlotsPanel->ClearChildren();
	SlotWidgets.Reset();

	const int32 SlotCount = LootViewModel->GetLootSlotCount();
	for (int32 i = 0; i < SlotCount; ++i)
	{
		UPBLootSlotWidget* SlotWidget = CreateWidget<UPBLootSlotWidget>(GetOwningPlayer(), LootSlotWidgetClass);
		if (!IsValid(SlotWidget))
		{
			continue;
		}

		LootSlotsPanel->AddChild(SlotWidget);
		SlotWidget->InitializeSlot(i, LootViewModel);
		SlotWidgets.Add(SlotWidget);
	}
}

void UPBLootPanelWidget::HandleLootSlotsRefreshed()
{
	RebuildSlotWidgets();
}

void UPBLootPanelWidget::OnCloseClicked()
{
	// 닫힘 이벤트 Broadcast (Action에서 바인딩하여 상호작용 종료 처리)
	OnLootClosed.Broadcast();

	// 위젯 자체를 Pop
	if (CachedPlayerController.IsValid())
	{
		UPBUIBlueprintLibrary::PopUI(CachedPlayerController.Get(), this);
	}
}

void UPBLootPanelWidget::OnTakeAllClicked()
{
	if (IsValid(LootViewModel))
	{
		LootViewModel->TakeAllItems();
	}
}
