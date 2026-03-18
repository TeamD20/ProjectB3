// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillBarViewModel.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/PBUITags.h"
#include "ProjectB3/UI/PBUITypes.h"

UPBSkillBarViewModel::UPBSkillBarViewModel()
{
	ViewModelTag = PBUITags::UI_ViewModel_SkillBar;
}

void UPBSkillBarViewModel::InitializeForPlayer(ULocalPlayer* InLocalPlayer)
{
	Super::InitializeForPlayer(InLocalPlayer);

	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		BindToPlayerState(PlayerController->GetPlayerState<APBGameplayPlayerState>());
	}
}

void UPBSkillBarViewModel::Deinitialize()
{
	BindToPlayerState(nullptr);
	PrimaryActions.Empty();
	SecondaryActions.Empty();
	SpellActions.Empty();
	ResponseActions.Empty();

	Super::Deinitialize();
}

void UPBSkillBarViewModel::BindToPlayerState(APBGameplayPlayerState* InPlayerState)
{
	if (PlayerState.IsValid())
	{
		if (SelectedPartyMemberChangedHandle.IsValid())
		{
			PlayerState->OnSelectedPartyMemberChanged.Remove(SelectedPartyMemberChangedHandle);
			SelectedPartyMemberChangedHandle.Reset();
		}
		if (PartyMembersChangedHandle.IsValid())
		{
			PlayerState->OnPartyMembersChanged.Remove(PartyMembersChangedHandle);
			PartyMembersChangedHandle.Reset();
		}
	}

	PlayerState = InPlayerState;

	if (PlayerState.IsValid())
	{
		SelectedPartyMemberChangedHandle = PlayerState->OnSelectedPartyMemberChanged.AddUObject(
			this,
			&UPBSkillBarViewModel::HandleSelectedPartyMemberChanged);
		PartyMembersChangedHandle = PlayerState->OnPartyMembersChanged.AddUObject(
			this,
			&UPBSkillBarViewModel::HandlePartyMembersChanged);
		RefreshFromCharacter(PlayerState->GetSelectedPartyMember());
		return;
	}

	RefreshFromCharacter(nullptr);
}

void UPBSkillBarViewModel::RefreshFromCharacter(AActor* InCharacter)
{
	PrimaryActions.Empty();
	SecondaryActions.Empty();
	SpellActions.Empty();
	ResponseActions.Empty();

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(InCharacter);
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface != nullptr
		? AbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;

	if (IsValid(AbilitySystemComponent))
	{
		// 1. 주행동 (Action)
		FGameplayTagContainer ActionRequireTags;
		ActionRequireTags.AddTag(PBGameplayTags::Ability_Type_Action);
		FGameplayTagContainer ActionIgnoreTags;
		ActionIgnoreTags.AddTag(PBGameplayTags::Ability_Spell);
		BuildSlotsFromFilter(AbilitySystemComponent, ActionRequireTags, ActionIgnoreTags, PrimaryActions);

		// 2. 보조행동 (BonusAction)
		FGameplayTagContainer BonusActionRequireTags;
		BonusActionRequireTags.AddTag(PBGameplayTags::Ability_Type_BonusAction);
		BuildSlotsFromFilter(AbilitySystemComponent, BonusActionRequireTags, FGameplayTagContainer(), SecondaryActions);

		// 3. 마법 (Spell)
		FGameplayTagContainer SpellRequireTags;
		SpellRequireTags.AddTag(PBGameplayTags::Ability_Spell); 
		BuildSlotsFromFilter(AbilitySystemComponent, SpellRequireTags, FGameplayTagContainer(), SpellActions);

		// 4. 대응 (Reaction)
		FGameplayTagContainer ReactionRequireTags;
		ReactionRequireTags.AddTag(PBGameplayTags::Ability_Type_Reaction);
		BuildSlotsFromFilter(AbilitySystemComponent, ReactionRequireTags, FGameplayTagContainer(), ResponseActions);
	}

	OnSlotsChanged.Broadcast();
}

void UPBSkillBarViewModel::RefreshAllCooldowns()
{
	UAbilitySystemComponent* AbilitySystemComponent = GetSelectedAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	auto RefreshSlotArray = [this, AbilitySystemComponent](TArray<FPBSkillSlotData>& Slots, int32 CategoryIndex)
	{
		const UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(AbilitySystemComponent);
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			FPBSkillSlotData& Slot = Slots[SlotIndex];
			Slot.bCanActivate = IsValid(PBASC) && PBASC->CanActivateAbilityByHandle(Slot.AbilityHandle);
			Slot.CooldownRemaining = IsValid(PBASC) ? PBASC->GetRemainingCooldown(Slot.AbilityHandle) : 0;
			OnSlotUpdated.Broadcast(CategoryIndex, SlotIndex);
		}
	};

	RefreshSlotArray(PrimaryActions, 0);
	RefreshSlotArray(SecondaryActions, 1);
	RefreshSlotArray(SpellActions, 2);
	RefreshSlotArray(ResponseActions, 3);
}

bool UPBSkillBarViewModel::GetSlotData(int32 CategoryIndex, int32 SlotIndex, FPBSkillSlotData& OutSlotData) const
{
	const TArray<FPBSkillSlotData>* Slots = GetSlotsByCategory(CategoryIndex);
	if (Slots == nullptr || !Slots->IsValidIndex(SlotIndex))
	{
		return false;
	}

	OutSlotData = (*Slots)[SlotIndex];
	return true;
}

const TArray<FPBSkillSlotData>* UPBSkillBarViewModel::GetSlotsByCategory(int32 CategoryIndex) const
{
	switch (CategoryIndex)
	{
	case 0: return &PrimaryActions;
	case 1: return &SecondaryActions;
	case 2: return &SpellActions;
	case 3: return &ResponseActions;
	default: return nullptr;
	}
}

void UPBSkillBarViewModel::HandleSelectedPartyMemberChanged(AActor* NewSelectedPartyMember)
{
	RefreshFromCharacter(NewSelectedPartyMember);
}

void UPBSkillBarViewModel::HandlePartyMembersChanged()
{
	RefreshFromCharacter(PlayerState.IsValid() ? PlayerState->GetSelectedPartyMember() : nullptr);
}

void UPBSkillBarViewModel::BuildSlotsFromFilter(
	UAbilitySystemComponent* AbilitySystemComponent,
	const FGameplayTagContainer& RequireTags,
	const FGameplayTagContainer& IgnoreTags,
	TArray<FPBSkillSlotData>& OutSlots) const
{
	const UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(AbilitySystemComponent);
	if (!IsValid(PBASC))
	{
		return;
	}

	const TArray<FGameplayAbilitySpecHandle> AbilityHandles = PBASC->GetAbilitySpecHandlesByTagFilter(RequireTags, IgnoreTags);
	OutSlots.Reserve(AbilityHandles.Num());

	for (const FGameplayAbilitySpecHandle AbilityHandle : AbilityHandles)
	{
		const FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromHandle(AbilityHandle);
		if (AbilitySpec == nullptr || !IsValid(AbilitySpec->Ability))
		{
			continue;
		}

		const UPBGameplayAbility* PBAbilityCDO = Cast<UPBGameplayAbility>(AbilitySpec->Ability);
		if (!IsValid(PBAbilityCDO))
		{
			continue;
		}

		FPBSkillSlotData SlotData;
		SlotData.AbilityHandle = AbilityHandle;
		SlotData.DisplayName = PBAbilityCDO->GetAbilityDisplayName().IsEmpty()
			? FText::FromString(PBAbilityCDO->GetName())
			: PBAbilityCDO->GetAbilityDisplayName();
		SlotData.Icon = PBAbilityCDO->GetAbilityIcon();
		SlotData.AbilityType = PBAbilityCDO->GetAbilityType(AbilityHandle, PBASC->AbilityActorInfo.Get());
		SlotData.CooldownRemaining = PBASC->GetRemainingCooldown(AbilityHandle);
		SlotData.bCanActivate = PBASC->CanActivateAbilityByHandle(AbilityHandle);
		OutSlots.Add(SlotData);
	}
}

UAbilitySystemComponent* UPBSkillBarViewModel::GetSelectedAbilitySystemComponent() const
{
	if (!PlayerState.IsValid())
	{
		return nullptr;
	}

	AActor* SelectedPartyMember = PlayerState->GetSelectedPartyMember();
	if (!IsValid(SelectedPartyMember))
	{
		return nullptr;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(SelectedPartyMember);
	if (AbilitySystemInterface == nullptr)
	{
		return nullptr;
	}

	return AbilitySystemInterface->GetAbilitySystemComponent();
}
