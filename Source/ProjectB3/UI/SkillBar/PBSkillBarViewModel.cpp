// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillBarViewModel.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility_Targeted.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/PBUITags.h"
#include "ProjectB3/UI/PBUITypes.h"

UPBSkillBarViewModel::UPBSkillBarViewModel()
{
	ViewModelTag = PBUITags::UI_ViewModel_SkillBar;
	bIsVisible = true;
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
		BuildSlotsFromFilter(AbilitySystemComponent, ActionRequireTags, ActionIgnoreTags, FText::FromString(TEXT("주 행동")), PrimaryActions);

		// 2. 보조행동 (BonusAction)
		FGameplayTagContainer BonusActionRequireTags;
		BonusActionRequireTags.AddTag(PBGameplayTags::Ability_Type_BonusAction);
		BuildSlotsFromFilter(AbilitySystemComponent, BonusActionRequireTags, FGameplayTagContainer(), FText::FromString(TEXT("보조 행동")), SecondaryActions);

		// 3. 마법 (Spell)
		FGameplayTagContainer SpellRequireTags;
		SpellRequireTags.AddTag(PBGameplayTags::Ability_Spell); 
		BuildSlotsFromFilter(AbilitySystemComponent, SpellRequireTags, FGameplayTagContainer(), FText::FromString(TEXT("주문")), SpellActions);

		// 4. 대응 (Reaction)
		FGameplayTagContainer ReactionRequireTags;
		ReactionRequireTags.AddTag(PBGameplayTags::Ability_Type_Reaction);
		BuildSlotsFromFilter(AbilitySystemComponent, ReactionRequireTags, FGameplayTagContainer(), FText::FromString(TEXT("대응")), ResponseActions);
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
	const FText& InSkillType,
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

		// --- Tooltip Data Binding ---
		SlotData.Description = PBAbilityCDO->GetAbilityDescription();
		SlotData.SkillType = InSkillType;

		// 사거리 (Range) 추출: Targeted 어빌리티인 경우에만 가져옴
		if (const UPBGameplayAbility_Targeted* TargetedCDO = Cast<UPBGameplayAbility_Targeted>(PBAbilityCDO))
		{
			if (TargetedCDO->IsRangedAbility())
			{
				SlotData.ActionRange = FText::FromString(FString::Printf(TEXT("%.0fm"), TargetedCDO->GetRange() / 100.f)); // cm -> m 로 변환 가정 (기획 의도에 맞게 수정 가능)
			}
			else
			{
				SlotData.ActionRange = FText::FromString(TEXT("인접"));
			}
		}

		// 주사위 정보 추출
		const FPBDiceSpec& DiceSpec = PBAbilityCDO->GetDiceSpec();
		SlotData.RollTypeEnum = DiceSpec.RollType;

		// DamageDesc 자동 계산 (예: 1~10 피해, 1~10 회복)
		EPBAbilityCategory Category = PBAbilityCDO->GetAbilityCategory();
		if (Category == EPBAbilityCategory::Heal)
		{
			SlotData.DamageDesc = FText::FromString(FString::Printf(TEXT("%d~%d 회복"), DiceSpec.DiceCount, DiceSpec.DiceCount * DiceSpec.DiceFaces));
		}
		else
		{
			SlotData.DamageDesc = FText::FromString(FString::Printf(TEXT("%d~%d 피해"), DiceSpec.DiceCount, DiceSpec.DiceCount * DiceSpec.DiceFaces));
		}

		// 디자이너 지정 설명이 있으면 우선, 없으면 자동 생성 ("1d10")
		SlotData.DiceRollDesc = FText::FromString(FString::Printf(TEXT("%dd%d"), DiceSpec.DiceCount, DiceSpec.DiceFaces));

		if (DiceSpec.RollType == EPBDiceRollType::HitRoll)
		{
			SlotData.RollType = FText::FromString(TEXT("명중 굴림"));
		}
		else if (DiceSpec.RollType == EPBDiceRollType::SavingThrow)
		{
			SlotData.RollType = FText::FromString(TEXT("내성 굴림"));
		}
		else if (DiceSpec.RollType == EPBDiceRollType::None)
		{
			SlotData.RollType = FText::FromString(TEXT("자동 적중"));
		}

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
