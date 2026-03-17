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
#include "ProjectB3/UI/Common/PBCombatStatsViewModel.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"

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
	BindProgressTurnDelegate();
}

void UPBAbilitySystemUIBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindProgressTurnDelegate();
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

	// Movement / MaxMovement 바인딩
	BindAttributeDelegate(UPBTurnResourceAttributeSet::GetMovementAttribute(), &ThisClass::HandleMovementChanged);
	BindAttributeDelegate(UPBTurnResourceAttributeSet::GetMaxMovementAttribute(), &ThisClass::HandleMovementChanged);

	// 전투 어트리뷰트 바인딩
	BindAttributeDelegate(UPBCharacterAttributeSet::GetArmorClassAttribute(), &ThisClass::HandleArmorClassChanged);
	BindAttributeDelegate(UPBCharacterAttributeSet::GetHitBonusAttribute(), &ThisClass::HandleHitBonusChanged);
	BindAttributeDelegate(UPBCharacterAttributeSet::GetSpellSaveDCModifierAttribute(), &ThisClass::HandleSpellSaveDCChanged);
	BindAttributeDelegate(UPBCharacterAttributeSet::GetProficiencyBonusAttribute(), &ThisClass::HandleSpellSaveDCChanged);

	// 초기값 즉시 반영
	HandleHPChanged(FOnAttributeChangeData());
	HandleMovementChanged(FOnAttributeChangeData());
	HandleArmorClassChanged(FOnAttributeChangeData());
	HandleHitBonusChanged(FOnAttributeChangeData());
	HandleSpellSaveDCChanged(FOnAttributeChangeData());
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

// === CombatStats 바인딩 ===

void UPBAbilitySystemUIBridge::HandleMovementChanged(const FOnAttributeChangeData& Data)
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

	float CurMovement = ASC->GetNumericAttribute(UPBTurnResourceAttributeSet::GetMovementAttribute());
	float CurMaxMovement = ASC->GetNumericAttribute(UPBTurnResourceAttributeSet::GetMaxMovementAttribute());

	UPBCombatStatsViewModel* CombatStatsVM = VMSubsystem->GetOrCreateActorViewModel<UPBCombatStatsViewModel>(GetOwner());
	if (IsValid(CombatStatsVM))
	{
		CombatStatsVM->SetMovement(CurMovement, CurMaxMovement);
	}
}

void UPBAbilitySystemUIBridge::HandleArmorClassChanged(const FOnAttributeChangeData& Data)
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

	int32 AC = FMath::FloorToInt32(ASC->GetNumericAttribute(UPBCharacterAttributeSet::GetArmorClassAttribute()));

	UPBCombatStatsViewModel* CombatStatsVM = VMSubsystem->GetOrCreateActorViewModel<UPBCombatStatsViewModel>(GetOwner());
	if (IsValid(CombatStatsVM))
	{
		CombatStatsVM->SetArmorClass(AC);
	}
}

void UPBAbilitySystemUIBridge::HandleHitBonusChanged(const FOnAttributeChangeData& Data)
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

	int32 Hit = FMath::FloorToInt32(ASC->GetNumericAttribute(UPBCharacterAttributeSet::GetHitBonusAttribute()));

	UPBCombatStatsViewModel* CombatStatsVM = VMSubsystem->GetOrCreateActorViewModel<UPBCombatStatsViewModel>(GetOwner());
	if (IsValid(CombatStatsVM))
	{
		CombatStatsVM->SetHitBonus(Hit);
	}
}

void UPBAbilitySystemUIBridge::HandleSpellSaveDCChanged(const FOnAttributeChangeData& Data)
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

	int32 Proficiency = FMath::FloorToInt32(ASC->GetNumericAttribute(UPBCharacterAttributeSet::GetProficiencyBonusAttribute()));
	int32 Modifier = FMath::FloorToInt32(ASC->GetNumericAttribute(UPBCharacterAttributeSet::GetSpellSaveDCModifierAttribute()));
	int32 DC = 8 + Proficiency + Modifier;

	UPBCombatStatsViewModel* CombatStatsVM = VMSubsystem->GetOrCreateActorViewModel<UPBCombatStatsViewModel>(GetOwner());
	if (IsValid(CombatStatsVM))
	{
		CombatStatsVM->SetSpellSaveDC(DC);
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

	// 어빌리티 종료 시 쿨다운이 적용되므로 스킬바 쿨다운 갱신
	HandleProgressTurnCompleted();
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

// === 턴 진행 바인딩 ===

void UPBAbilitySystemUIBridge::BindProgressTurnDelegate()
{
	UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(CachedASC.Get());
	if (!IsValid(PBASC))
	{
		return;
	}

	ProgressTurnHandle = PBASC->OnProgressTurnCompleted.AddUObject(
		this, &ThisClass::HandleProgressTurnCompleted);
}

void UPBAbilitySystemUIBridge::UnbindProgressTurnDelegate()
{
	UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(CachedASC.Get());
	if (!IsValid(PBASC))
	{
		return;
	}

	if (ProgressTurnHandle.IsValid())
	{
		PBASC->OnProgressTurnCompleted.Remove(ProgressTurnHandle);
		ProgressTurnHandle.Reset();
	}
}

void UPBAbilitySystemUIBridge::HandleProgressTurnCompleted()
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

	// 현재 선택된 파티원이 아니면 갱신 불필요
	APBGameplayPlayerState* PS = SkillBarVM->GetPlayerState();
	if (IsValid(PS) && PS->GetSelectedPartyMember() != GetOwner())
	{
		return;
	}

	SkillBarVM->RefreshAllCooldowns();
}
