// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/MainActionBar/PBMainActionBarHUD.h"
#include "ProjectB3/UI/SkillBar/PBSkillBarViewModel.h"
#include "ProjectB3/UI/SkillBar/PBEquipmentSlotWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewModel.h"
#include "ProjectB3/UI/MainActionBar/PBProfileWidget.h"
#include "ProjectB3/UI/MainActionBar/PBResponseSkillWidget.h"
#include "Components/Button.h"

void UPBMainActionBarHUD::NativeConstruct()
{
	Super::NativeConstruct();
	
	SkillBarViewModel = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBSkillBarViewModel>(GetOwningLocalPlayer());
	if (IsValid(SkillBarViewModel))
	{
		SlotsChangedHandle = SkillBarViewModel->OnSlotsChanged.AddUObject(this, &UPBMainActionBarHUD::HandleSlotsChanged);
		RefreshEquipmentSlots();
	}

	CachedPlayerState = GetOwningPlayerPawn() ? GetOwningPlayerPawn()->GetPlayerState<APBGameplayPlayerState>() : nullptr;
	if (IsValid(CachedPlayerState))
	{
		SelectedPartyMemberChangedHandle = CachedPlayerState->OnSelectedPartyMemberChanged.AddUObject(this, &UPBMainActionBarHUD::HandleSelectedPartyMemberChanged);
		HandleSelectedPartyMemberChanged(CachedPlayerState->GetSelectedPartyMember());
	}

	if (InventoryButton)
	{
		InventoryButton->OnClicked.AddDynamic(this, &UPBMainActionBarHUD::OnInventoryButtonClicked);
	}

	if (ResponseSkillArea && SkillBarViewModel)
	{
		ResponseSkillArea->InitializeResponseArea(SkillBarViewModel);
	}
}

void UPBMainActionBarHUD::NativeDestruct()
{
	if (IsValid(SkillBarViewModel))
	{
		if (SlotsChangedHandle.IsValid())
		{
			SkillBarViewModel->OnSlotsChanged.Remove(SlotsChangedHandle);
			SlotsChangedHandle.Reset();
		}
	}

	SkillBarViewModel = nullptr;

	if (IsValid(CachedPlayerState) && SelectedPartyMemberChangedHandle.IsValid())
	{
		CachedPlayerState->OnSelectedPartyMemberChanged.Remove(SelectedPartyMemberChangedHandle);
		SelectedPartyMemberChangedHandle.Reset();
	}
	CachedPlayerState = nullptr;

	Super::NativeDestruct();
}

void UPBMainActionBarHUD::HandleSlotsChanged()
{
	RefreshEquipmentSlots();
}

void UPBMainActionBarHUD::HandleSelectedPartyMemberChanged(AActor* NewActor)
{
	if (IsValid(ProfileArea) && IsValid(NewActor))
	{
		if (UPBPartyMemberViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetOwningLocalPlayer(), NewActor))
		{
			ProfileArea->InitializeProfile(VM);
		}
	}
}

void UPBMainActionBarHUD::OnInventoryButtonClicked()
{
	// 인벤토리 열기 공통 처리 (사운드 연동 등)
	// 필요 시 블루프린트로 이벤트 전파
}

void UPBMainActionBarHUD::RefreshEquipmentSlots()
{
	if (!IsValid(SkillBarViewModel))
	{
		return;
	}

	// 주무기 슬롯 수동 연동
	if (MainWeaponSlot1 && SkillBarViewModel->WeaponSlots.IsValidIndex(0))
		MainWeaponSlot1->UpdateSlot(SkillBarViewModel->WeaponSlots[0]);
	
	if (MainWeaponSlot2 && SkillBarViewModel->WeaponSlots.IsValidIndex(1))
		MainWeaponSlot2->UpdateSlot(SkillBarViewModel->WeaponSlots[1]);

	// 유틸리티 슬롯 수동 연동
	if (UtilitySlot1 && SkillBarViewModel->UtilitySlots.IsValidIndex(0))
		UtilitySlot1->UpdateSlot(SkillBarViewModel->UtilitySlots[0]);
	
	if (UtilitySlot2 && SkillBarViewModel->UtilitySlots.IsValidIndex(1))
		UtilitySlot2->UpdateSlot(SkillBarViewModel->UtilitySlots[1]);

	if (UtilitySlot3 && SkillBarViewModel->UtilitySlots.IsValidIndex(2))
		UtilitySlot3->UpdateSlot(SkillBarViewModel->UtilitySlots[2]);

	if (UtilitySlot4 && SkillBarViewModel->UtilitySlots.IsValidIndex(3))
		UtilitySlot4->UpdateSlot(SkillBarViewModel->UtilitySlots[3]);
}
