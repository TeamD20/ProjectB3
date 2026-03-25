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
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"
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
	ConsumableActions.Empty();
	ResponseActions.Empty();

	if (CachedInventory.IsValid())
	{
		CachedInventory->OnInventoryItemChanged.RemoveDynamic(this, &UPBSkillBarViewModel::HandleInventoryItemChanged);
		CachedInventory->OnInventoryFullRefresh.RemoveDynamic(this, &UPBSkillBarViewModel::HandleInventoryFullRefresh);
	}
	CachedInventory.Reset();

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
	ConsumableActions.Empty();
	ResponseActions.Empty();

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(InCharacter);
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface != nullptr
		? AbilitySystemInterface->GetAbilitySystemComponent()
		: nullptr;

	if (IsValid(AbilitySystemComponent))
	{
		// 1. 주행동 (Action) + 마법 (Spell) 통합
		FGameplayTagContainer ActionRequireTags;
		ActionRequireTags.AddTag(PBGameplayTags::Ability_Type_Action);
		FGameplayTagContainer ActionIgnoreTags; // Spells are NOT ignored
		BuildSlotsFromFilter(AbilitySystemComponent, ActionRequireTags, ActionIgnoreTags, FText::FromString(TEXT("주 행동")), PrimaryActions);

		// 마법 전용 (Ability.Spell을 가지면서 Action 태그가 없는 어빌리티를 커버하기 위한 추가 작업)
		FGameplayTagContainer SpellRequireTags;
		SpellRequireTags.AddTag(PBGameplayTags::Ability_Spell); 
		FGameplayTagContainer SpellIgnoreTags;
		SpellIgnoreTags.AddTag(PBGameplayTags::Ability_Type_Action); // 주행동에서 이미 추가된 것은 제외
		BuildSlotsFromFilter(AbilitySystemComponent, SpellRequireTags, SpellIgnoreTags, FText::FromString(TEXT("주문")), PrimaryActions);

		// 2. 보조행동 (BonusAction)
		FGameplayTagContainer BonusActionRequireTags;
		BonusActionRequireTags.AddTag(PBGameplayTags::Ability_Type_BonusAction);
		BuildSlotsFromFilter(AbilitySystemComponent, BonusActionRequireTags, FGameplayTagContainer(), FText::FromString(TEXT("보조 행동")), SecondaryActions);

		// 3. 대응 (Reaction)
		FGameplayTagContainer ReactionRequireTags;
		ReactionRequireTags.AddTag(PBGameplayTags::Ability_Type_Reaction);
		BuildSlotsFromFilter(AbilitySystemComponent, ReactionRequireTags, FGameplayTagContainer(), FText::FromString(TEXT("대응")), ResponseActions);
	}

	UPBInventoryComponent* InventoryComponent = InCharacter ? InCharacter->FindComponentByClass<UPBInventoryComponent>() : nullptr;
	if (CachedInventory.IsValid() && CachedInventory.Get() != InventoryComponent)
	{
		CachedInventory->OnInventoryItemChanged.RemoveDynamic(this, &UPBSkillBarViewModel::HandleInventoryItemChanged);
		CachedInventory->OnInventoryFullRefresh.RemoveDynamic(this, &UPBSkillBarViewModel::HandleInventoryFullRefresh);
	}

	CachedInventory = InventoryComponent;

	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryItemChanged.RemoveDynamic(this, &UPBSkillBarViewModel::HandleInventoryItemChanged);
		InventoryComponent->OnInventoryItemChanged.AddDynamic(this, &UPBSkillBarViewModel::HandleInventoryItemChanged);
		InventoryComponent->OnInventoryFullRefresh.RemoveDynamic(this, &UPBSkillBarViewModel::HandleInventoryFullRefresh);
		InventoryComponent->OnInventoryFullRefresh.AddDynamic(this, &UPBSkillBarViewModel::HandleInventoryFullRefresh);
	}

	RefreshConsumables();
}

void UPBSkillBarViewModel::RefreshConsumables()
{
	ConsumableActions.Empty();
	if (CachedInventory.IsValid())
	{
		const TArray<FPBItemInstance>& AllItems = CachedInventory->GetItems();
		for (const FPBItemInstance& Item : AllItems)
		{
			if (Item.ItemDataAsset && Item.ItemDataAsset->ItemType == EPBItemType::Consumable && ConsumableActions.Num() < 4)
			{
				FPBSkillSlotData SlotData;
				
				// 소비아이템 UI 렌더링에 필요한 정보 구성
				SlotData.Description = Item.ItemDataAsset->BackGroundDescription;
				SlotData.DisplayName = Item.ItemDataAsset->ItemName;
				SlotData.Icon = Item.ItemDataAsset->ItemIcon;
				SlotData.SkillType = FText::FromString(TEXT("소비 아이템"));
				SlotData.AbilityType = EPBAbilityType::Free;
				SlotData.bCanActivate = true; 
				SlotData.CooldownRemaining = 0;
				// 어빌리티가 아닌 아이템 식별용 인스턴스 ID 할당
				SlotData.ItemInstanceID = Item.InstanceID;
				
				ConsumableActions.Add(SlotData);
			}
			
			if (ConsumableActions.Num() >= 4)
			{
				break;
			}
		}
	}

	OnSlotsChanged.Broadcast();
}

void UPBSkillBarViewModel::HandleInventoryItemChanged(int32 SlotIndex)
{
	RefreshConsumables();
}

void UPBSkillBarViewModel::HandleInventoryFullRefresh()
{
	RefreshConsumables();
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
	RefreshSlotArray(ConsumableActions, 2);
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
	case 2: return &ConsumableActions;
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

		// DamageDesc 및 가독성 최적화 (0 피해 방어코드 + DiceBonus 반영)
		EPBAbilityCategory Category = PBAbilityCDO->GetAbilityCategory();
		const int32 Bonus = DiceSpec.DiceBonus;
		int32 MinDamage = FMath::Max(0, DiceSpec.DiceCount + Bonus);
		int32 MaxDamage = FMath::Max(0, DiceSpec.DiceCount * DiceSpec.DiceFaces + Bonus);

		auto FormatDamageDesc = [](int32 MinVal, int32 MaxVal, const TCHAR* Suffix) -> FText
		{
			if (MinVal == 0 && MaxVal == 0)
			{
				return FText::FromString(FString::Printf(TEXT("0 %s"), Suffix));
			}
			else if (MinVal == MaxVal)
			{
				return FText::FromString(FString::Printf(TEXT("%d %s"), MaxVal, Suffix));
			}
			return FText::FromString(FString::Printf(TEXT("%d~%d %s"), MinVal, MaxVal, Suffix));
		};

		if (Category == EPBAbilityCategory::Heal)
		{
			SlotData.DamageDesc = FormatDamageDesc(MinDamage, MaxDamage, TEXT("회복"));
		}
		else
		{
			SlotData.DamageDesc = FormatDamageDesc(MinDamage, MaxDamage, TEXT("피해"));
		}

		// 주사위 표기 숨김 조건:
		// 1) 주사위 수/면체가 0인 경우 (피해 없음)
		// 2) 1면체(1d1)인 경우 — 고정 데미지이므로 주사위·피해 표기 모두 불필요
		if ((DiceSpec.DiceCount == 0 && DiceSpec.DiceFaces == 0) || DiceSpec.DiceFaces == 1)
		{
			SlotData.DiceRollDesc = FText::GetEmpty();
			SlotData.DamageDesc = FText::GetEmpty();
		}
		else
		{
			// 보너스가 있으면 "1d6+3", 없으면 "1d6"
			if (Bonus > 0)
			{
				SlotData.DiceRollDesc = FText::FromString(FString::Printf(TEXT("%dd%d+%d"), DiceSpec.DiceCount, DiceSpec.DiceFaces, Bonus));
			}
			else if (Bonus < 0)
			{
				SlotData.DiceRollDesc = FText::FromString(FString::Printf(TEXT("%dd%d%d"), DiceSpec.DiceCount, DiceSpec.DiceFaces, Bonus));
			}
			else
			{
				SlotData.DiceRollDesc = FText::FromString(FString::Printf(TEXT("%dd%d"), DiceSpec.DiceCount, DiceSpec.DiceFaces));
			}
		}

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
