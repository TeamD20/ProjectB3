// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (파티 공용 인벤토리 컨테이너 UI 작성)

#include "PBPartyInventoryContainerWidget.h"

#include "Components/HorizontalBox.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/Inventory/PBInventoryPanelWidget.h"
#include "ProjectB3/UI/Inventory/PBInventoryViewModel.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"

void UPBPartyInventoryContainerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// PlayerState를 기준으로 파티 패널 생명주기를 관리
	CachePlayerState();
	if (IsValid(CachedPlayerState))
	{
		PartyMembersChangedHandle = CachedPlayerState->OnPartyMembersChanged.AddUObject(this, &ThisClass::HandlePartyMembersChanged);
	}

	RebuildPanels();
}

void UPBPartyInventoryContainerWidget::NativeDestruct()
{
	// 파티 변경 이벤트 구독을 해제해 중복 Rebuild를 방지
	if (IsValid(CachedPlayerState) && PartyMembersChangedHandle.IsValid())
	{
		CachedPlayerState->OnPartyMembersChanged.Remove(PartyMembersChangedHandle);
		PartyMembersChangedHandle.Reset();
	}

	CachedPlayerState = nullptr;
	ActivePanels.Reset();

	Super::NativeDestruct();
}

void UPBPartyInventoryContainerWidget::RebuildPanels()
{
	ActivePanels.Reset();

	if (!IsValid(PartyPanelBox) || !IsValid(PanelWidgetClass))
	{
		return;
	}

	PartyPanelBox->ClearChildren();

	if (!IsValid(CachedPlayerState))
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return;
	}

	// 파티원마다 Actor-Bound VM + PanelWidget을 1:1로 생성
	const TArray<AActor*> PartyMembers = CachedPlayerState->GetPartyMembers();
	for (AActor* PartyMember : PartyMembers)
	{
		if (!IsValid(PartyMember))
		{
			continue;
		}

		UPBInventoryViewModel* InventoryViewModel = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBInventoryViewModel>(LocalPlayer, PartyMember);
		if (!IsValid(InventoryViewModel))
		{
			continue;
		}

		UPBInventoryPanelWidget* PanelWidget = CreateWidget<UPBInventoryPanelWidget>(this, PanelWidgetClass);
		if (!IsValid(PanelWidget))
		{
			continue;
		}

		PanelWidget->InitializeWithViewModel(InventoryViewModel);
		PartyPanelBox->AddChild(PanelWidget);
		ActivePanels.Add(PanelWidget);
	}
}

void UPBPartyInventoryContainerWidget::CachePlayerState()
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	CachedPlayerState = IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<APBGameplayPlayerState>() : nullptr;
}

void UPBPartyInventoryContainerWidget::HandlePartyMembersChanged()
{
	RebuildPanels();
}
