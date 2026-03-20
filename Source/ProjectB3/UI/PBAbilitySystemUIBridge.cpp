// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySystemUIBridge.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"
#include "ProjectB3/AbilitySystem/Payload/PBFloatingTextPayload.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnPortraitViewModel.h"
#include "ProjectB3/UI/SkillBar/PBSkillBarViewModel.h"
#include "ProjectB3/UI/Common/PBCombatStatsViewModel.h"
#include "ProjectB3/UI/CombatLog/PBCombatLogViewModel.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySystemRegistry.h"
#include "ProjectB3/UI/Combat/PBCombatStateTextActor.h"
#include "ProjectB3/UI/Combat/PBCombatStateTextWidget.h"
#include "ProjectB3/UI/Combat/PBSkillNameFloatingActor.h"
#include "ProjectB3/UI/Combat/PBSkillNameFloatingWidget.h"
#include "ProjectB3/UI/Combat/PBActionIndicatorViewModel.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Game/PBGameInstance.h"

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
	BindCombatResultDelegates();
	BindCombatStateDelegate();
}

void UPBAbilitySystemUIBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindCombatStateDelegate();
	UnbindCombatResultDelegates();
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
	UPBTurnPortraitViewModel* TurnVM = VMSubsystem->GetOrCreateActorViewModel<UPBTurnPortraitViewModel>(GetOwner());
	if (IsValid(TurnVM))
	{
		float Percent = (CurMaxHP > 0) ? static_cast<float>(CurHP) / static_cast<float>(CurMaxHP) : 0.f;
		TurnVM->SetHealthPercent(Percent);
	}

	// HP가 0 이하로 내려가면 사망 로그 전송
	if (CurHP <= 0)
	{
		FText Name = GetTargetDisplayName();
		SendCombatLogEntry(EPBCombatLogType::Death,
			FText::Format(NSLOCTEXT("PBCombatLog", "Death", "{0}이(가) 쓰러졌습니다."), Name));
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
	const UPBGameplayAbility* PBAbility = Cast<UPBGameplayAbility>(Ability);
	if (!IsValid(PBAbility))
	{
		return;
	}

	FGameplayAbilitySpecHandle Handle = PBAbility->GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = PBAbility->GetCurrentActorInfo();
	UpdateSkillSlotActiveState(Handle, true);

	// 스킬 이름 플로팅 UI 스폰
	EPBAbilityType AbilityType = PBAbility->GetAbilityType(Handle, ActorInfo);
	const FText AbilityName = PBAbility->GetAbilityDisplayName();
	
	if (!AbilityName.IsEmpty())
	{
		if (UWorld* World = GetWorld())
		{
			if (UPBCombatManagerSubsystem* CombatManager = World->GetSubsystem<UPBCombatManagerSubsystem>())
			{
				CombatManager->OnSkillActivated.Broadcast(GetOwner(), AbilityName, AbilityType);
			}
		}
		
		// 행동 인디케이터 갱신 (스킬 시전 중)
		UpdateActionIndicator(EPBActionIndicatorType::SkillCast, FText::Format(NSLOCTEXT("PBUI", "CastingTarget", "{0} 시전 중"), AbilityName));
	}

	// 어빌리티 사용 로그
	if (AbilityType != EPBAbilityType::None)
	{
		if (!AbilityName.IsEmpty())
		{
			SendCombatLogEntry(EPBCombatLogType::System,
				FText::Format(NSLOCTEXT("PBCombatLog", "AbilityActivated", "{0}이(가) {1}을(를) 사용했습니다."),
					GetTargetDisplayName(), AbilityName));
		}
	}
}

void UPBAbilitySystemUIBridge::HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	const UPBGameplayAbility* PBAbility = Cast<UPBGameplayAbility>(AbilityEndedData.AbilityThatEnded);
	if (!IsValid(PBAbility))
	{
		return;
	}
	

	FGameplayAbilitySpecHandle Handle = PBAbility->GetCurrentAbilitySpecHandle();
	const FGameplayAbilityActorInfo* ActorInfo = PBAbility->GetCurrentActorInfo();
	UpdateSkillSlotActiveState(Handle, false);

	// 행동 인디케이터 초기화 (스킬 시전 종료)
	ClearActionIndicator();

	// 어빌리티 종료 시 쿨다운이 적용되므로 스킬바 쿨다운 갱신
	HandleProgressTurnCompleted();

	// 취소된 경우에만 로그 출력
	if (AbilityEndedData.bWasCancelled)
	{
		if (PBAbility->GetAbilityType(Handle, ActorInfo) != EPBAbilityType::None)
		{
			const FText AbilityName = PBAbility->GetAbilityDisplayName();
			if (!AbilityName.IsEmpty())
			{
				SendCombatLogEntry(EPBCombatLogType::System,
					FText::Format(NSLOCTEXT("PBCombatLog", "AbilityCancelled", "{0}의 {1}이(가) 취소되었습니다."),
						GetTargetDisplayName(), AbilityName));
			}
		}
	}
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

// === 전투 결과 바인딩 ===

void UPBAbilitySystemUIBridge::BindCombatResultDelegates()
{
	UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(CachedASC.Get());
	if (!IsValid(PBASC))
	{
		return;
	}

	GEExecutedHandle = PBASC->OnGEExecuted.AddUObject(this, &ThisClass::HandleGEExecuted);
	TagUpdatedHandle = PBASC->OnGameplayTagUpdated.AddUObject(this, &ThisClass::HandleTagUpdated);
}

void UPBAbilitySystemUIBridge::UnbindCombatResultDelegates()
{
	UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(CachedASC.Get());
	if (!IsValid(PBASC))
	{
		return;
	}

	if (GEExecutedHandle.IsValid())
	{
		PBASC->OnGEExecuted.Remove(GEExecutedHandle);
		GEExecutedHandle.Reset();
	}

	if (TagUpdatedHandle.IsValid())
	{
		PBASC->OnGameplayTagUpdated.Remove(TagUpdatedHandle);
		TagUpdatedHandle.Reset();
	}
}

void UPBAbilitySystemUIBridge::HandleGEExecuted(const FGameplayEffectSpec& Spec, const FGameplayAttribute& Attribute, float EffectiveValue)
{
	FGameplayTagContainer AssetTags;
	Spec.GetAllAssetTags(AssetTags);
	const bool bMiss = AssetTags.HasTag(PBGameplayTags::Combat_Result_Miss);
	const bool bSaveSuccess = AssetTags.HasTag(PBGameplayTags::Combat_Result_Save_Success);
	const bool bSaveFailed = AssetTags.HasTag(PBGameplayTags::Combat_Result_Save_Failed);

	bool bCritical = AssetTags.HasTag(PBGameplayTags::Combat_Hit_Critical);
	if (!bCritical)
	{
		FGameplayTagContainer SourceTags = Spec.CapturedSourceTags.GetSpecTags();
		bCritical = SourceTags.HasTag(PBGameplayTags::Combat_Hit_Critical);
	}

	if (Attribute == UPBCharacterAttributeSet::GetIncomingDamageAttribute())
	{
		if (bMiss)
		{
			SendFloatingTextEvent(EPBFloatingTextType::Miss, 0.f);
			SendCombatLogEntry(EPBCombatLogType::Miss,
				FText::Format(NSLOCTEXT("PBCombatLog", "Miss", "{0}이(가) 공격을 회피했습니다."), GetTargetDisplayName()));
			return;
		}

		if (bSaveSuccess)
		{
			SendFloatingTextEvent(EPBFloatingTextType::SaveSuccess, 0.f);
			SendCombatLogEntry(EPBCombatLogType::SaveSuccess,
				FText::Format(NSLOCTEXT("PBCombatLog", "SaveSuccess", "{0}이(가) 내성에 성공했습니다."), GetTargetDisplayName()));
		}
		else if (bSaveFailed)
		{
			SendFloatingTextEvent(EPBFloatingTextType::SaveFailed, 0.f);
			SendCombatLogEntry(EPBCombatLogType::SaveFailed,
				FText::Format(NSLOCTEXT("PBCombatLog", "SaveFailed", "{0}이(가) 내성에 실패했습니다."), GetTargetDisplayName()));
		}

		if (EffectiveValue > 0.f)
		{
			SendFloatingTextEvent(
				EPBFloatingTextType::Damage,
				-EffectiveValue,
				bCritical ? PBGameplayTags::Combat_Hit_Critical : FGameplayTag());

			const FText SourceName = GetSourceDisplayName(Spec);
			const FText TargetName = GetTargetDisplayName();
			const FText DamageValue = FText::AsNumber(FMath::FloorToInt(EffectiveValue));

			if (bCritical)
			{
				SendCombatLogEntry(EPBCombatLogType::CritDamage,
					FText::Format(NSLOCTEXT("PBCombatLog", "CritDamage", "{0}이(가) {1}에게 치명타! {2}의 피해."),
						SourceName, TargetName, DamageValue));
			}
			else
			{
				SendCombatLogEntry(EPBCombatLogType::Damage,
					FText::Format(NSLOCTEXT("PBCombatLog", "Damage", "{0}이(가) {1}에게 {2}의 피해를 입혔습니다."),
						SourceName, TargetName, DamageValue));
			}
		}
		return;
	}

	if (Attribute == UPBCharacterAttributeSet::GetIncomingHealAttribute() && EffectiveValue > 0.f)
	{
		SendFloatingTextEvent(EPBFloatingTextType::Heal, EffectiveValue);
		SendCombatLogEntry(EPBCombatLogType::Heal,
			FText::Format(NSLOCTEXT("PBCombatLog", "Heal", "{0}이(가) {1} HP를 회복했습니다."),
				GetTargetDisplayName(), FText::AsNumber(FMath::FloorToInt(EffectiveValue))));
	}
}

void UPBAbilitySystemUIBridge::HandleTagUpdated(const FGameplayTag& Tag, bool bTagExists)
{
	SendFloatingTextEvent(EPBFloatingTextType::Status, 0.f, Tag);

	// Registry에서 태그의 표시 이름 조회 — 없으면 로그 전송 생략
	const UPBAbilitySystemRegistry* Registry = UPBGameInstance::GetAbilitySystemRegistry(this);
	if (!IsValid(Registry))
	{
		return;
	}

	const FText StatusName = Registry->GetTagDisplayName(Tag);
	if (StatusName.IsEmpty())
	{
		return;
	}

	const FText TargetName = GetTargetDisplayName();
	if (bTagExists)
	{
		SendCombatLogEntry(EPBCombatLogType::Status,
			FText::Format(NSLOCTEXT("PBCombatLog", "StatusAdded", "{0}에게 {1}이(가) 부여되었습니다."),
				TargetName, StatusName));
	}
	else
	{
		SendCombatLogEntry(EPBCombatLogType::Status,
			FText::Format(NSLOCTEXT("PBCombatLog", "StatusRemoved", "{0}의 {1}이(가) 해제되었습니다."),
				TargetName, StatusName));
	}
}

void UPBAbilitySystemUIBridge::SendFloatingTextEvent(
	EPBFloatingTextType Type,
	float Magnitude,
	const FGameplayTag& MetaTag,
	const FText& TextOverride) const
{
	UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(CachedASC.Get());
	if (!IsValid(PBASC))
	{
		return;
	}

	UPBFloatingTextPayload* Payload = NewObject<UPBFloatingTextPayload>(GetOwner());
	if (!IsValid(Payload))
	{
		return;
	}

	Payload->FloatingTextType = Type;
	Payload->Magnitude = Magnitude;
	Payload->MetaTag = MetaTag;

	switch (Type)
	{
	case EPBFloatingTextType::Damage:
		break;
	case EPBFloatingTextType::Heal:
		break;
	case EPBFloatingTextType::Miss:
		Payload->Text = NSLOCTEXT("PBFloatingText", "Miss", "명중 실패");
		break;
	case EPBFloatingTextType::SaveSuccess:
		Payload->Text = NSLOCTEXT("PBFloatingText", "SaveSuccess", "내성 굴림 성공");
		break;
	case EPBFloatingTextType::SaveFailed:
		Payload->Text = NSLOCTEXT("PBFloatingText", "SaveFailed", "내성 굴림 실패");
		break;
	case EPBFloatingTextType::Status:
		Payload->Text = TextOverride;
		break;
	default:
		break;
	}

	if (!TextOverride.IsEmpty() && Type != EPBFloatingTextType::Status)
	{
		Payload->Text = TextOverride;
	}

	FGameplayEventData EventData;
	EventData.EventTag = PBGameplayTags::Event_UI_FloatingText;
	EventData.Instigator = GetOwner();
	EventData.Target = GetOwner();
	EventData.OptionalObject = Payload;

	PBASC->HandleGameplayEvent(PBGameplayTags::Event_UI_FloatingText, &EventData);
}

void UPBAbilitySystemUIBridge::SendCombatLogEntry(EPBCombatLogType InLogType, const FText& LogText) const
{
	UPBViewModelSubsystem* VMSubsystem = GetViewModelSubsystem();
	if (!IsValid(VMSubsystem))
	{
		return;
	}

	UPBCombatLogViewModel* LogVM = VMSubsystem->GetOrCreateGlobalViewModel<UPBCombatLogViewModel>();
	if (!IsValid(LogVM))
	{
		return;
	}

	LogVM->AddEntry(InLogType, LogText);
}

FText UPBAbilitySystemUIBridge::GetTargetDisplayName() const
{
	if (IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(GetOwner()))
	{
		return CPI->GetCombatDisplayName();
	}

	return NSLOCTEXT("PBCombatLog", "UnknownTarget", "알 수 없는 대상");
}

FText UPBAbilitySystemUIBridge::GetSourceDisplayName(const FGameplayEffectSpec& Spec) const
{
	AActor* EffectCauser = Spec.GetContext().GetEffectCauser();
	if (IsValid(EffectCauser))
	{
		if (IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(EffectCauser))
		{
			return CPI->GetCombatDisplayName();
		}
	}

	return NSLOCTEXT("PBCombatLog", "UnknownSource", "알 수 없는 출처");
}

// === 전투 상태 및 스킬 플로팅 UI ===

void UPBAbilitySystemUIBridge::BindCombatStateDelegate()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPBCombatManagerSubsystem* CombatManager = World->GetSubsystem<UPBCombatManagerSubsystem>();
	if (!IsValid(CombatManager))
	{
		return;
	}

	// 전투 상태 변경 플로팅 UI 스폰은 HUD로 넘어갔으나, 브리지 내부 데이터 정리용으로 구독 유지
	CombatStateHandle = CombatManager->OnCombatStateChanged.AddUObject(this, &ThisClass::HandleCombatStateChanged);
}

void UPBAbilitySystemUIBridge::UnbindCombatStateDelegate()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	UPBCombatManagerSubsystem* CombatManager = World->GetSubsystem<UPBCombatManagerSubsystem>();
	if (IsValid(CombatManager) && CombatStateHandle.IsValid())
	{
		CombatManager->OnCombatStateChanged.Remove(CombatStateHandle);
		CombatStateHandle.Reset();
	}
}

void UPBAbilitySystemUIBridge::HandleCombatStateChanged(EPBCombatState NewState)
{
	// 전투 종료 시 브리지 내부의 인디케이터 잔재물이 있으면 정리
	if (NewState == EPBCombatState::CombatEnding)
	{
		ClearActionIndicator();
	}
}

// === 행동 인디케이터 UI ===

void UPBAbilitySystemUIBridge::UpdateActionIndicator(EPBActionIndicatorType Type, const FText& Text)
{
	UPBViewModelSubsystem* VMSubsystem = GetViewModelSubsystem();
	if (!IsValid(VMSubsystem))
	{
		return;
	}

	// 개별 대상마다 인디케이터 뷰모델이 있다.
	UPBActionIndicatorViewModel* VM = VMSubsystem->GetOrCreateActorViewModel<UPBActionIndicatorViewModel>(GetOwner());
	if (IsValid(VM))
	{
		FPBActionIndicatorData ActionData;
		ActionData.ActionType = Type;
		ActionData.DisplayText = Text;
		// ActionData.Icon = ... (필요 시 AbilityType 등에 기반해 설정)
		
		VM->SetAction(ActionData);
	}
}

void UPBAbilitySystemUIBridge::ClearActionIndicator()
{
	UPBViewModelSubsystem* VMSubsystem = GetViewModelSubsystem();
	if (!IsValid(VMSubsystem))
	{
		return;
	}

	UPBActionIndicatorViewModel* VM = VMSubsystem->FindActorViewModel<UPBActionIndicatorViewModel>(GetOwner());
	if (IsValid(VM))
	{
		VM->ClearAction();
	}
}
