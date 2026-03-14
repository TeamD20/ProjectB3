// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySystemUIBridge.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnPortraitViewModel.h"
#include "ProjectB3/UI/SkillBar/PBSkillBarViewModel.h"

UPBAbilitySystemUIBridge::UPBAbilitySystemUIBridge()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPBAbilitySystemUIBridge::BeginPlay()
{
	Super::BeginPlay();

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		CachedASC = ASI->GetAbilitySystemComponent();
	}

	if (!CachedASC.IsValid())
	{
		return;
	}

	BindAttributeDelegates();
	BindAbilityDelegates();
}

void UPBAbilitySystemUIBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAbilityDelegates();
	ClearAttributeBindings();

	Super::EndPlay(EndPlayReason);
}

UPBViewModelSubsystem* UPBAbilitySystemUIBridge::GetViewModelSubsystem() const
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = World->GetFirstLocalPlayerFromController();
	if (!IsValid(LocalPlayer))
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UPBViewModelSubsystem>();
}

// === Attribute 바인딩 ===

void UPBAbilitySystemUIBridge::BindAttributeDelegates()
{
	ClearAttributeBindings();

	if (!CachedASC.IsValid())
	{
		return;
	}

	// HP / MaxHP 바인딩
	BindAttributeDelegate(UPBCharacterAttributeSet::GetHPAttribute(), &ThisClass::HandleHPChanged);
	BindAttributeDelegate(UPBCharacterAttributeSet::GetMaxHPAttribute(), &ThisClass::HandleHPChanged);

	// 초기값 즉시 반영
	HandleHPChanged(FOnAttributeChangeData());
}

void UPBAbilitySystemUIBridge::ClearAttributeBindings()
{
	if (CachedASC.IsValid())
	{
		for (FAttributeBinding& Binding : AttributeBindings)
		{
			if (!Binding.IsValid())
			{
				continue;
			}
			CachedASC->GetGameplayAttributeValueChangeDelegate(Binding.Attribute).Remove(Binding.DelegateHandle);
		}
	}

	AttributeBindings.Reset();
}

void UPBAbilitySystemUIBridge::BindAttributeDelegate(
	const FGameplayAttribute& Attribute,
	void (UPBAbilitySystemUIBridge::*Handler)(const FOnAttributeChangeData&))
{
	if (!CachedASC.IsValid())
	{
		return;
	}

	FAttributeBinding NewBinding;
	NewBinding.Attribute = Attribute;
	NewBinding.DelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, Handler);
	AttributeBindings.Add(NewBinding);
}

void UPBAbilitySystemUIBridge::HandleHPChanged(const FOnAttributeChangeData& Data)
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!IsValid(ASC))
	{
		return;
	}

	UPBViewModelSubsystem* VMSubsystem = GetViewModelSubsystem();
	if (!IsValid(VMSubsystem))
	{
		return;
	}

	int32 CurHP = FMath::FloorToInt32(ASC->GetNumericAttribute(UPBCharacterAttributeSet::GetHPAttribute()));
	int32 CurMaxHP = FMath::FloorToInt32(ASC->GetNumericAttribute(UPBCharacterAttributeSet::GetMaxHPAttribute()));

	// 플레이어 진영: GetOrCreate (VM이 없으면 생성)
	// 그 외 진영: Find (VM이 이미 있을 때만 갱신)
	UPBPartyMemberViewModel* PartyVM = nullptr;
	if (IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(GetOwner()))
	{
		if (CPI->GetFactionTag().MatchesTagExact(PBGameplayTags::Combat_Faction_Player))
		{
			PartyVM = VMSubsystem->GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetOwner());
		}
		else
		{
			PartyVM = VMSubsystem->FindActorViewModel<UPBPartyMemberViewModel>(GetOwner());
		}
	}

	if (IsValid(PartyVM))
	{
		PartyVM->SetHP(CurHP, CurMaxHP);
	}

	// TurnPortrait는 전투 중에만 존재하므로 Find
	UPBTurnPortraitViewModel* TurnVM = VMSubsystem->FindActorViewModel<UPBTurnPortraitViewModel>(GetOwner());
	if (IsValid(TurnVM))
	{
		float Percent = (CurMaxHP > 0) ? static_cast<float>(CurHP) / static_cast<float>(CurMaxHP) : 0.f;
		TurnVM->SetHealthPercent(Percent);
	}
}

// === 어빌리티 상태 바인딩 ===

void UPBAbilitySystemUIBridge::BindAbilityDelegates()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!IsValid(ASC))
	{
		return;
	}

	AbilityActivatedHandle = ASC->AbilityActivatedCallbacks.AddUObject(
		this, &ThisClass::HandleAbilityActivated);

	AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(
		this, &ThisClass::HandleAbilityEnded);
}

void UPBAbilitySystemUIBridge::UnbindAbilityDelegates()
{
	UAbilitySystemComponent* ASC = CachedASC.Get();
	if (!IsValid(ASC))
	{
		return;
	}

	if (AbilityActivatedHandle.IsValid())
	{
		ASC->AbilityActivatedCallbacks.Remove(AbilityActivatedHandle);
		AbilityActivatedHandle.Reset();
	}

	if (AbilityEndedHandle.IsValid())
	{
		ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
		AbilityEndedHandle.Reset();
	}
}

void UPBAbilitySystemUIBridge::HandleAbilityActivated(UGameplayAbility* Ability)
{
	if (!IsValid(Ability))
	{
		return;
	}

	FGameplayAbilitySpecHandle Handle = Ability->GetCurrentAbilitySpecHandle();
	UpdateSkillSlotActiveState(Handle, true);
}

void UPBAbilitySystemUIBridge::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (!IsValid(AbilityEndedData.AbilityThatEnded))
	{
		return;
	}

	FGameplayAbilitySpecHandle Handle = AbilityEndedData.AbilityThatEnded->GetCurrentAbilitySpecHandle();
	UpdateSkillSlotActiveState(Handle, false);
}

void UPBAbilitySystemUIBridge::UpdateSkillSlotActiveState(FGameplayAbilitySpecHandle Handle, bool bActive)
{
	UPBViewModelSubsystem* VMSubsystem = GetViewModelSubsystem();
	if (!IsValid(VMSubsystem))
	{
		return;
	}

	UPBSkillBarViewModel* SkillBarVM = VMSubsystem->GetOrCreateGlobalViewModel<UPBSkillBarViewModel>();
	if (!IsValid(SkillBarVM))
	{
		return;
	}

	// 현재 선택된 파티원이 아니면 SkillBar 갱신 불필요
	APBGameplayPlayerState* PS = IsValid(SkillBarVM->GetPlayerState()) ? SkillBarVM->GetPlayerState() : nullptr;
	if (IsValid(PS))
	{
		if (PS->GetSelectedPartyMember() != GetOwner())
		{
			return;
		}
	}

	// 4개 카테고리 순회하여 Handle 매칭하는 슬롯의 bIsActive 갱신
	auto UpdateCategory = [&](TArray<FPBSkillSlotData>& Slots, int32 CategoryIndex)
	{
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (Slots[i].AbilityHandle == Handle)
			{
				Slots[i].bIsActive = bActive;
				SkillBarVM->OnSlotUpdated.Broadcast(CategoryIndex, i);
				return;
			}
		}
	};

	UpdateCategory(SkillBarVM->PrimaryActions, 0);
	UpdateCategory(SkillBarVM->SecondaryActions, 1);
	UpdateCategory(SkillBarVM->SpellActions, 2);
	UpdateCategory(SkillBarVM->ResponseActions, 3);
}
