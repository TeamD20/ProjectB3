// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Combat/PBCombatSystemLibrary.h"
#include "ProjectB3/Player/PBGameplayPlayerController.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/ItemSystem/PBEquipmentActor.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Data/PBEquipmentDataAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBGameplayAbility, Log, All);

UPBGameplayAbility::UPBGameplayAbility()
{
	DiceSpec.RollType = EPBDiceRollType::None;
}

APBCharacterBase* UPBGameplayAbility::K2_GetPBCharacter(EPBValidResult& Result) const
{
	APBCharacterBase* Character = GetPBCharacter();
	Result = IsValid(Character) ? EPBValidResult::Valid : EPBValidResult::Invalid;
	return Character;
}

APBCharacterBase* UPBGameplayAbility::GetPBCharacter() const
{
	return Cast<APBCharacterBase>(GetAvatarActorFromActorInfo());
}

APBGameplayPlayerController* UPBGameplayAbility::K2_GetPBPlayerController(EPBValidResult& Result) const
{
	APBGameplayPlayerController* Controller = GetPBPlayerController();
	Result = IsValid(Controller) ? EPBValidResult::Valid : EPBValidResult::Invalid;
	return Controller;
}

APBGameplayPlayerController* UPBGameplayAbility::GetPBPlayerController() const
{
	if (APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo()))
	{
		return Pawn->GetController<APBGameplayPlayerController>();
	}
	return nullptr;
}

UPBAbilitySystemComponent* UPBGameplayAbility::GetPBAbilitySystemComponent() const
{
	if (!CurrentActorInfo)
	{
		return nullptr;
	}
	return Cast<UPBAbilitySystemComponent>( CurrentActorInfo->AbilitySystemComponent.Get());
}

UPBAbilitySystemComponent* UPBGameplayAbility::GetPBAbilitySystemComponentFromActorInfo(
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (ActorInfo)
	{
		return Cast<UPBAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());	
	}
	return nullptr;
}


bool UPBGameplayAbility::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 타입이 None이면 제한 없이 활성화 허용
	if (GetAbilityType(Handle, ActorInfo) == EPBAbilityType::None)
	{
		return true;
	}

	// 턴 기반 어빌리티가 이미 실행 중이면 활성화 거부
	if (const UPBAbilitySystemComponent* PBASC = GetPBAbilitySystemComponentFromActorInfo(ActorInfo))
	{
		if (PBASC->IsTurnAbilityActive())
		{
			return false;
		}
	}

	return true;
}

void UPBGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 턴 기반 어빌리티인 경우 ASC에 실행 중 플래그 설정
	if (GetAbilityType(Handle, ActorInfo) != EPBAbilityType::None)
	{
		if (UPBAbilitySystemComponent* PBASC = GetPBAbilitySystemComponentFromActorInfo(ActorInfo))
		{
			PBASC->SetTurnAbilityActive(true);
		}
	}

	// 장비 슬롯 태그가 DynamicTag에 있으면 해당 무기를 캐릭터에 자동 부착
	TryAutoAttachEquipment(Handle, ActorInfo);

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPBGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 턴 기반 어빌리티인 경우
	if (GetAbilityType(Handle, ActorInfo) != EPBAbilityType::None)
	{
		if (UPBAbilitySystemComponent* PBASC = GetPBAbilitySystemComponentFromActorInfo(ActorInfo))
		{
			// ASC 플래그 해제
			PBASC->SetTurnAbilityActive(false);
		}
		
		// 전투중이 아니면 자유 이동모드로 복귀 
		if (!UPBCombatSystemLibrary::IsInCombat(this))
		{
			if (APBGameplayPlayerController* PBPC = GetPBPlayerController())
			{
				PBPC->SetControllerMode(EPBPlayerControllerMode::FreeMovement);
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UGameplayEffect* UPBGameplayAbility::GetCooldownGameplayEffect() const
{
	// TODO: 턴제 쿨다운은 GAS 내장 CooldownTag GE를 사용하지 않는다.
	// 추후 턴 종료 시점에 별도 쿨다운 카운터로 관리 예정.
	return nullptr;
}

EPBAbilityType UPBGameplayAbility::K2_GetAbilityType() const
{
	return GetAbilityType(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo());
}

EPBAbilityType UPBGameplayAbility::GetAbilityType(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!Handle.IsValid() || !ActorInfo)
	{
		// 활성 Spec이 없으면 AssetTags만 조회한다.
		return GetAbilityTypeFromTags(GetAssetTags());
	}

	// Valid한 Spec이 있는 경우 DynamicTag까지 조회
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			FGameplayTagContainer CombinedTags;
			CombinedTags.AppendTags(GetAssetTags());
			CombinedTags.AppendTags(Spec->GetDynamicSpecSourceTags());
			return GetAbilityTypeFromTags(CombinedTags);
		}
	}
	
	return GetAbilityTypeFromTags(GetAssetTags());
}

FPBHitRollResult UPBGameplayAbility::RollHit(const UAbilitySystemComponent* InTargetASC) const
{
	if (DiceSpec.RollType != EPBDiceRollType::HitRoll)
	{
		UE_LOG(LogPBGameplayAbility, Warning, TEXT("DiceSpec의 RollType이 HitRoll이 아니지만 RollHit()을 호출함"));
	}
	
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	// 명중 수정치(숙련 제외) + 숙련 보너스
	const int32 HitBonus = UPBAbilitySystemLibrary::GetHitBonus(SourceASC, DiceSpec.BonusAttributeOverride) 
		+ UPBAbilitySystemLibrary::GetProficiencyBonus(SourceASC);

	bool bFound = false;
	const float TargetACValue = IsValid(InTargetASC)
		? InTargetASC->GetGameplayAttributeValue(UPBCharacterAttributeSet::GetArmorClassAttribute(), bFound)
		: 0.f;
	const int32 TargetAC = static_cast<int32>(TargetACValue);

	const FPBHitRollResult HitRollResult = UPBAbilitySystemLibrary::RollHit(HitBonus, TargetAC);
	UE_LOG(LogPBGameplayAbility, Warning,
		TEXT("[%s] RollHit - Roll: %d, HitBonus: %d, TargetAC: %d, bHit: %s, bCritical: %s"),
		*GetName(),
		HitRollResult.Roll,
		HitBonus,
		TargetAC,
		HitRollResult.bHit ? TEXT("true") : TEXT("false"),
		HitRollResult.bCritical ? TEXT("true") : TEXT("false"));

	// TODO: UI 시스템 연동
	return HitRollResult;
}

FPBDamageRollResult UPBGameplayAbility::RollDamage(bool bCritical) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAttribute AttackModifierAttributeOverride = DiceSpec.AttackModifierAttributeOverride;
	// AttackModifier = KeyAttribute 수정치 (숙련 보너스는 데미지에 미포함)
	const float AttackModifier =  UPBAbilitySystemLibrary::GetAttackModifier(SourceASC, AttackModifierAttributeOverride);
	const FPBDamageRollResult DamageRollResult = UPBAbilitySystemLibrary::RollDamage(DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, bCritical);
	UE_LOG(LogPBGameplayAbility, Warning,
		TEXT("[%s] RollDamage - Dice: %dd%d, bCritical: %s, DiceRoll: %.2f, AttackModifier: %.2f"),
		*GetName(),
		DiceSpec.DiceCount,
		DiceSpec.DiceFaces,
		bCritical ? TEXT("true") : TEXT("false"),
		DamageRollResult.DiceRoll,
		DamageRollResult.AttackModifier);

	// TODO: UI 시스템 연동
	return DamageRollResult;
}

FPBSavingThrowResult UPBGameplayAbility::RollSavingThrow(const UAbilitySystemComponent* InTargetASC) const
{
	if (DiceSpec.RollType != EPBDiceRollType::SavingThrow)
	{
		UE_LOG(LogPBGameplayAbility, Warning, TEXT("DiceSpec의 RollType이 SavingThrow 아니지만 RollSavingThrow()을 호출함"));
	}
	
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	// SaveBonus = SaveAttributeOverride 능력치 수정치 (숙련 제외)
	const int32 SaveBonus = UPBAbilitySystemLibrary::GetSaveBonus(InTargetASC, DiceSpec.TargetSaveAttribute);
	const int32 SpellSaveDC = UPBAbilitySystemLibrary::CalcSpellSaveDC(SourceASC, DiceSpec.BonusAttributeOverride);
	const FPBSavingThrowResult SavingThrowResult = UPBAbilitySystemLibrary::RollSavingThrow(SaveBonus, SpellSaveDC);
	UE_LOG(LogPBGameplayAbility, Warning,
		TEXT("[%s] RollSavingThrow - Roll: %d, SaveBonus: %d, SpellSaveDC: %d, bSucceeded: %s"),
		*GetName(),
		SavingThrowResult.Roll,
		SaveBonus,
		SpellSaveDC,
		SavingThrowResult.bSucceeded ? TEXT("true") : TEXT("false"));

	// TODO: UI 시스템 연동
	return SavingThrowResult;
}

FGameplayEffectSpecHandle UPBGameplayAbility::MakeDamageEffectSpecFromHitDamageRoll(
	const UAbilitySystemComponent* InTargetASC, FPBHitRollResult& OutHitRollResult,
	FPBDamageRollResult& OutDamageRollResult) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC) || !IsValid(InTargetASC))
	{
		return FGameplayEffectSpecHandle();
	}

	const TSubclassOf<UGameplayEffect> DamageGEClass = UPBAbilitySystemLibrary::GetDamageGEClass(SourceASC);
	if (!DamageGEClass)
	{
		return FGameplayEffectSpecHandle();
	}

	const FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGEClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}
	
	// 명중 굴림
	OutHitRollResult = RollHit(InTargetASC);
	if (!OutHitRollResult.bHit)
	{
		return FGameplayEffectSpecHandle();
	}
	
	// 데미지 굴림
	OutDamageRollResult = RollDamage(OutHitRollResult.bCritical);
	
	// 스펙에 반영
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_DiceRoll,      OutDamageRollResult.DiceRoll);
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_AttackModifier, OutDamageRollResult.AttackModifier);
	
	return SpecHandle;
}

FGameplayEffectSpecHandle UPBGameplayAbility::MakeDamageEffectSpecFromSavingThrowDamageRoll(
	const UAbilitySystemComponent* InTargetASC, FPBSavingThrowResult& OutSavingThrowResult,
	FPBDamageRollResult& OutDamageRollResult) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(SourceASC) || !IsValid(InTargetASC))
	{
		return FGameplayEffectSpecHandle();
	}

	const TSubclassOf<UGameplayEffect> DamageGEClass = UPBAbilitySystemLibrary::GetDamageGEClass(SourceASC);
	if (!DamageGEClass)
	{
		return FGameplayEffectSpecHandle();
	}
	
	const FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageGEClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}
	
	// 데미지 굴림
	OutDamageRollResult = RollDamage(false);
	// 내성 굴림
	OutSavingThrowResult = RollSavingThrow(InTargetASC);
	if (OutSavingThrowResult.bSucceeded)
	{
		OutDamageRollResult.DiceRoll *= 0.5f;
	}
	
	// 스펙에 반영
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_DiceRoll,      OutDamageRollResult.DiceRoll);
	SpecHandle.Data->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Damage_AttackModifier, OutDamageRollResult.AttackModifier);
	
	return SpecHandle;
}

float UPBGameplayAbility::GetExpectedHitDamage(const UAbilitySystemComponent* InTargetASC,
                                               const FGameplayTagContainer& SourceTags,
                                               const FGameplayTagContainer& TargetTags) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	// AttackModifier: 데미지 수정치 (숙련 미포함)
	const float AttackModifier = static_cast<float>(UPBAbilitySystemLibrary::GetAttackModifier(SourceASC,  DiceSpec.AttackModifierAttributeOverride));
	// HitBonus: 명중 수정치 + 숙련 보너스
	const int32 HitBonus = UPBAbilitySystemLibrary::GetHitBonus(SourceASC, DiceSpec.BonusAttributeOverride)
		+ UPBAbilitySystemLibrary::GetProficiencyBonus(SourceASC);
	
	bool bFound = false;
	const float TargetACValue = IsValid(InTargetASC)
		? InTargetASC->GetGameplayAttributeValue(UPBCharacterAttributeSet::GetArmorClassAttribute(), bFound)
		: 0.f;
	const int32 TargetAC = static_cast<int32>(TargetACValue);

	return UPBAbilitySystemLibrary::CalcExpectedAttackDamage(
		DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, HitBonus, TargetAC, SourceTags, TargetTags);
}

float UPBGameplayAbility::GetExpectedSavingThrowDamage(const UAbilitySystemComponent* InTargetASC,
                                                       const FGameplayTagContainer& SourceTags,
                                                       const FGameplayTagContainer& TargetTags) const
{
	const UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const float AttackModifier = static_cast<float>(UPBAbilitySystemLibrary::GetAttackModifier(SourceASC, DiceSpec.AttackModifierAttributeOverride));
	const int32 SaveBonus = UPBAbilitySystemLibrary::GetSaveBonus(InTargetASC, DiceSpec.TargetSaveAttribute);
	const int32 SpellSaveDC = UPBAbilitySystemLibrary::CalcSpellSaveDC(SourceASC, DiceSpec.BonusAttributeOverride);
	
	return UPBAbilitySystemLibrary::CalcExpectedSavingThrowDamage(
		DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, SaveBonus, SpellSaveDC, SourceTags, TargetTags);
}

float UPBGameplayAbility::GetExpectedDirectDamage(
	const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& TargetTags) const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float AttackModifier = static_cast<float>(UPBAbilitySystemLibrary::GetAttackModifier(ASC,  DiceSpec.AttackModifierAttributeOverride));
	
	return UPBAbilitySystemLibrary::CalcExpectedDamage(
		DiceSpec.DiceCount, DiceSpec.DiceFaces, AttackModifier, SourceTags, TargetTags);
}

void UPBGameplayAbility::TryAutoAttachEquipment(
	const FGameplayAbilitySpecHandle& Handle,
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!IsValid(ASC))
	{
		return;
	}

	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		return;
	}

	// DynamicTags에서 Equipment.Slot 태그 탐색
	const FGameplayTagContainer& DynamicTags = Spec->GetDynamicSpecSourceTags();
	FGameplayTag FoundSlotTag;
	for (const FGameplayTag& Tag : DynamicTags)
	{
		if (Tag.MatchesTag(PBGameplayTags::Equipment_Slot))
		{
			FoundSlotTag = Tag;
			break;
		}
	}

	if (!FoundSlotTag.IsValid())
	{
		return;
	}

	// 캐릭터와 EquipmentComponent 조회
	APBCharacterBase* Character = Cast<APBCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!IsValid(Character))
	{
		return;
	}

	UPBEquipmentComponent* EquipComp = Character->FindComponentByClass<UPBEquipmentComponent>();
	if (!IsValid(EquipComp))
	{
		return;
	}

	// 해당 슬롯 태그에 대응하는 장착 아이템의 EquipmentActorClass 조회
	for (const auto& Pair : EquipComp->GetEquippedItems())
	{
		const UPBEquipmentDataAsset* EquipData = Cast<UPBEquipmentDataAsset>(Pair.Value.ItemDataAsset);
		if (!IsValid(EquipData) || EquipData->EquipmentActorClass.IsNull())
		{
			continue;
		}

		// 이 슬롯의 부착 태그가 찾은 태그와 일치하는지 확인
		const FGameplayTag SlotTag = EquipData->AttachSlotOverride.IsValid() ? EquipData->AttachSlotOverride : UPBEquipmentComponent::GetAttachSlotTag(Pair.Key);
		if (SlotTag == FoundSlotTag)
		{
			FGameplayTag AttachSlotTag = FoundSlotTag;
			const TSubclassOf<APBEquipmentActor> LoadedClass = EquipData->EquipmentActorClass.LoadSynchronous();
			if (LoadedClass)
			{
				Character->AttachEquipment(AttachSlotTag, LoadedClass);
			}
			break;
		}
	}
}

FPBAbilityTargetData UPBGameplayAbility::ExtractTargetDataFromEvent(
	const FGameplayEventData& EventData) const
{
	if (const UPBTargetPayload* Payload = Cast<UPBTargetPayload>(EventData.OptionalObject))
	{
		return Payload->TargetData;
	}

	return FPBAbilityTargetData();
}
