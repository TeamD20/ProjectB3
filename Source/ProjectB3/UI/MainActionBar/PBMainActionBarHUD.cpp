// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/MainActionBar/PBMainActionBarHUD.h"
#include "ProjectB3/UI/SkillBar/PBSkillBarViewModel.h"
#include "ProjectB3/UI/SkillBar/PBEquipmentSlotWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewModel.h"
#include "ProjectB3/UI/MainActionBar/PBProfileWidget.h"
#include "ProjectB3/UI/MainActionBar/PBResponseSkillWidget.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/UI/Common/PBCombatStatsViewModel.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"

void UPBMainActionBarHUD::NativeConstruct()
{
	Super::NativeConstruct();

	SkillBarViewModel = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBSkillBarViewModel>(GetOwningLocalPlayer());
	if (IsValid(SkillBarViewModel))
	{
		BindVisibilityToViewModel(SkillBarViewModel);
		SlotsChangedHandle = SkillBarViewModel->OnSlotsChanged.AddUObject(this, &UPBMainActionBarHUD::HandleSlotsChanged);
		RefreshEquipmentSlots();
	}

	CachedPlayerState = GetOwningPlayerPawn() ? GetOwningPlayerPawn()->GetPlayerState<APBGameplayPlayerState>() : nullptr;
	if (IsValid(CachedPlayerState))
	{
		SelectedPartyMemberChangedHandle = CachedPlayerState->OnSelectedPartyMemberChanged.AddUObject(this, &UPBMainActionBarHUD::HandleSelectedPartyMemberChanged);
		HandleSelectedPartyMemberChanged(CachedPlayerState->GetSelectedPartyMember());
	}

	if (IsValid(InventoryButton))
	{
		InventoryButton->OnClicked.AddDynamic(this, &UPBMainActionBarHUD::OnInventoryButtonClicked);
	}

	if (IsValid(TurnEndButton))
	{
		TurnEndButton->OnClicked.AddDynamic(this, &UPBMainActionBarHUD::OnTurnEndButtonClicked);
		TurnEndButton->SetIsEnabled(false);
	}

	if (UPBCombatManagerSubsystem* CombatMgr = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		ActiveTurnChangedHandle = CombatMgr->OnActiveTurnChanged.AddUObject(this, &UPBMainActionBarHUD::HandleActiveTurnChanged);
		CombatStateChangedHandle = CombatMgr->OnCombatStateChanged.AddUObject(this, &UPBMainActionBarHUD::HandleCombatStateChanged);
		UpdateTurnEndButtonState();
	}

	if (IsValid(ResponseSkillArea) && IsValid(SkillBarViewModel))
	{
		ResponseSkillArea->InitializeResponseArea(SkillBarViewModel);
	}
}

void UPBMainActionBarHUD::NativeDestruct()
{
	if (IsValid(SkillBarViewModel))
	{
		UnbindVisibilityFromViewModel(SkillBarViewModel);
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

	if (IsValid(CachedCombatStatsVM) && MovementPercentChangedHandle.IsValid())
	{
		CachedCombatStatsVM->OnMovementPercentChanged.Remove(MovementPercentChangedHandle);
		MovementPercentChangedHandle.Reset();
	}
	CachedCombatStatsVM = nullptr;

	if (UPBCombatManagerSubsystem* CombatMgr = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		if (ActiveTurnChangedHandle.IsValid())
		{
			CombatMgr->OnActiveTurnChanged.Remove(ActiveTurnChangedHandle);
			ActiveTurnChangedHandle.Reset();
		}
		if (CombatStateChangedHandle.IsValid())
		{
			CombatMgr->OnCombatStateChanged.Remove(CombatStateChangedHandle);
			CombatStateChangedHandle.Reset();
		}
	}

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

	BindCombatStatsViewModel(NewActor);
}

void UPBMainActionBarHUD::OnInventoryButtonClicked()
{
}

void UPBMainActionBarHUD::RefreshEquipmentSlots()
{
	if (!IsValid(SkillBarViewModel))
	{
		return;
	}

	if (IsValid(MainWeaponSlot1) && SkillBarViewModel->WeaponSlots.IsValidIndex(0))
	{
		MainWeaponSlot1->UpdateSlot(SkillBarViewModel->WeaponSlots[0]);
	}

	if (IsValid(MainWeaponSlot2) && SkillBarViewModel->WeaponSlots.IsValidIndex(1))
	{
		MainWeaponSlot2->UpdateSlot(SkillBarViewModel->WeaponSlots[1]);
	}

	if (IsValid(UtilitySlot1) && SkillBarViewModel->UtilitySlots.IsValidIndex(0))
	{
		UtilitySlot1->UpdateSlot(SkillBarViewModel->UtilitySlots[0]);
	}

	if (IsValid(UtilitySlot2) && SkillBarViewModel->UtilitySlots.IsValidIndex(1))
	{
		UtilitySlot2->UpdateSlot(SkillBarViewModel->UtilitySlots[1]);
	}

	if (IsValid(UtilitySlot3) && SkillBarViewModel->UtilitySlots.IsValidIndex(2))
	{
		UtilitySlot3->UpdateSlot(SkillBarViewModel->UtilitySlots[2]);
	}

	if (IsValid(UtilitySlot4) && SkillBarViewModel->UtilitySlots.IsValidIndex(3))
	{
		UtilitySlot4->UpdateSlot(SkillBarViewModel->UtilitySlots[3]);
	}
}

void UPBMainActionBarHUD::OnTurnEndButtonClicked()
{
	if (UPBCombatManagerSubsystem* CombatMgr = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		if (!CombatMgr->IsInCombat() || CombatMgr->GetCombatState() != EPBCombatState::TurnInProgress)
		{
			return;
		}

		AActor* CurrentCombatant = CombatMgr->GetCurrentCombatant();
		if (!IsValid(CurrentCombatant))
		{
			return;
		}

		bool bIsPlayerTurn = false;
		if (const IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(CurrentCombatant))
		{
			bIsPlayerTurn = CPI->GetFactionTag().MatchesTagExact(PBGameplayTags::Combat_Faction_Player);
		}
		
		if (bIsPlayerTurn)
		{
			CombatMgr->EndCurrentTurn();
		}
	}
}

void UPBMainActionBarHUD::HandleActiveTurnChanged(AActor* Combatant, int32 TurnIndex)
{
	UpdateTurnEndButtonState();
}

void UPBMainActionBarHUD::HandleCombatStateChanged(EPBCombatState NewState)
{
	UpdateTurnEndButtonState();
}

void UPBMainActionBarHUD::UpdateTurnEndButtonState()
{
	if (!IsValid(TurnEndButton))
	{
		return;
	}

	UPBCombatManagerSubsystem* CombatMgr = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
	if (!IsValid(CombatMgr) || !CombatMgr->IsInCombat() || CombatMgr->GetCombatState() != EPBCombatState::TurnInProgress)
	{
		TurnEndButton->SetIsEnabled(false);
		return;
	}

	AActor* CurrentCombatant = CombatMgr->GetCurrentCombatant();
	if (!IsValid(CurrentCombatant))
	{
		TurnEndButton->SetIsEnabled(false);
		return;
	}

	bool bIsPlayerTurn = false;
	if (const IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(CurrentCombatant))
	{
		bIsPlayerTurn = CPI->GetFactionTag().MatchesTagExact(PBGameplayTags::Combat_Faction_Player);
	}

	TurnEndButton->SetIsEnabled(bIsPlayerTurn);
}

void UPBMainActionBarHUD::BindCombatStatsViewModel(AActor* NewActor)
{
	// 기존 바인딩 해제
	if (IsValid(CachedCombatStatsVM) && MovementPercentChangedHandle.IsValid())
	{
		CachedCombatStatsVM->OnMovementPercentChanged.Remove(MovementPercentChangedHandle);
		MovementPercentChangedHandle.Reset();
	}
	CachedCombatStatsVM = nullptr;

	if (!IsValid(NewActor))
	{
		if (IsValid(MovementDistanceProgressBar))
		{
			MovementDistanceProgressBar->SetPercent(0.f);
		}
		return;
	}

	CachedCombatStatsVM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBCombatStatsViewModel>(GetOwningLocalPlayer(), NewActor);
	if (IsValid(CachedCombatStatsVM))
	{
		MovementPercentChangedHandle = CachedCombatStatsVM->OnMovementPercentChanged.AddUObject(
			this, &UPBMainActionBarHUD::HandleMovementPercentChanged);

		// 초기값 반영
		HandleMovementPercentChanged(CachedCombatStatsVM->GetMovementPercent());
	}
}

void UPBMainActionBarHUD::HandleMovementPercentChanged(float NewPercent)
{
	if (IsValid(MovementDistanceProgressBar))
	{
		MovementDistanceProgressBar->SetPercent(NewPercent);
	}
}
