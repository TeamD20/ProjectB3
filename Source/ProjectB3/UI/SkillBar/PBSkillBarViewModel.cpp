// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillBarViewModel.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"

namespace PBSkillBarTabIndex
{
	static constexpr int32 Action = 0;
	static constexpr int32 BonusAction = 1;
	static constexpr int32 Spell = 2;
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
	ActionSlots.Empty();
	BonusActionSlots.Empty();
	SpellSlots.Empty();

	Super::Deinitialize();
}

void UPBSkillBarViewModel::BindToPlayerState(APBGameplayPlayerState* InPlayerState)
{
	if (PlayerState.IsValid() && SelectedPartyMemberChangedHandle.IsValid())
	{
		PlayerState->OnSelectedPartyMemberChanged.Remove(SelectedPartyMemberChangedHandle);
		SelectedPartyMemberChangedHandle.Reset();
	}

	PlayerState = InPlayerState;

	if (PlayerState.IsValid())
	{
		SelectedPartyMemberChangedHandle = PlayerState->OnSelectedPartyMemberChanged.AddUObject(
			this,
			&UPBSkillBarViewModel::HandleSelectedPartyMemberChanged);
		RefreshFromCharacter(PlayerState->GetSelectedPartyMember());
		return;
	}

	RefreshFromCharacter(nullptr);
}

void UPBSkillBarViewModel::RefreshFromCharacter(AActor* InCharacter)
{
	ActionSlots.Empty();
	BonusActionSlots.Empty();
	SpellSlots.Empty();

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(InCharacter);
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface != nullptr
		? AbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;

	if (IsValid(AbilitySystemComponent))
	{
		FGameplayTagContainer ActionRequireTags;
		ActionRequireTags.AddTag(PBGameplayTags::Ability_Type_Action);

		FGameplayTagContainer ActionIgnoreTags;
		ActionIgnoreTags.AddTag(PBGameplayTags::Ability_Spell);
		BuildSlotsFromFilter(AbilitySystemComponent, ActionRequireTags, ActionIgnoreTags, ActionSlots);

		FGameplayTagContainer BonusActionRequireTags;
		BonusActionRequireTags.AddTag(PBGameplayTags::Ability_Type_BonusAction);
		BuildSlotsFromFilter(AbilitySystemComponent, BonusActionRequireTags, FGameplayTagContainer(), BonusActionSlots);

		FGameplayTagContainer SpellRequireTags;
		SpellRequireTags.AddTag(PBGameplayTags::Ability_Spell);
		BuildSlotsFromFilter(AbilitySystemComponent, SpellRequireTags, FGameplayTagContainer(), SpellSlots);
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

	auto RefreshSlotArray = [this, AbilitySystemComponent](TArray<FPBSkillSlotData>& Slots, int32 TabIndex)
	{
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			FPBSkillSlotData& Slot = Slots[SlotIndex];
			Slot.bCanActivate = AbilitySystemComponent->CanActivateAbility(Slot.AbilityHandle);
			OnSlotUpdated.Broadcast(TabIndex, SlotIndex);
		}
	};

	RefreshSlotArray(ActionSlots, PBSkillBarTabIndex::Action);
	RefreshSlotArray(BonusActionSlots, PBSkillBarTabIndex::BonusAction);
	RefreshSlotArray(SpellSlots, PBSkillBarTabIndex::Spell);
}

bool UPBSkillBarViewModel::GetSlotData(int32 TabIndex, int32 SlotIndex, FPBSkillSlotData& OutSlotData) const
{
	const TArray<FPBSkillSlotData>* Slots = GetSlotsByTab(TabIndex);
	if (Slots == nullptr || !Slots->IsValidIndex(SlotIndex))
	{
		return false;
	}

	OutSlotData = (*Slots)[SlotIndex];
	return true;
}

const TArray<FPBSkillSlotData>* UPBSkillBarViewModel::GetSlotsByTab(int32 TabIndex) const
{
	switch (TabIndex)
	{
	case PBSkillBarTabIndex::Action:
		return &ActionSlots;
	case PBSkillBarTabIndex::BonusAction:
		return &BonusActionSlots;
	case PBSkillBarTabIndex::Spell:
		return &SpellSlots;
	default:
		return nullptr;
	}
}

void UPBSkillBarViewModel::HandleSelectedPartyMemberChanged(AActor* NewSelectedPartyMember)
{
	RefreshFromCharacter(NewSelectedPartyMember);
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
		SlotData.AbilityType = PBAbilityCDO->GetAbilityType();
		SlotData.CooldownRemaining = 0;
		SlotData.bCanActivate = AbilitySystemComponent->CanActivateAbility(AbilityHandle);
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
