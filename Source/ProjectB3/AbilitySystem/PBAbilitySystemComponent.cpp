// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySystemComponent.h"

#include "Attributes/PBTurnResourceAttributeSet.h"
#include "Components/PBTurnEffectComponent.h"
#include "Data/PBAbilitySetData.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Combat/PBCombatSystemLibrary.h"

DEFINE_LOG_CATEGORY(LogPBAbilitySystem);

UPBAbilitySystemComponent::UPBAbilitySystemComponent()
{
}

void UPBAbilitySystemComponent::GrantAbilitiesFromData(
	const FGameplayTag& SourceTag,
	const UPBAbilitySetData* Data,
	int32 CharacterLevel)
{
	if (!IsValid(Data))
	{
		UE_LOG(LogPBAbilitySystem, Warning,
			TEXT("GrantAbilitiesFromData: Data가 유효하지 않습니다. (Source: %s)"),
			*SourceTag.ToString());
		return;
	}

	// 중복 호출 방어 — 기존 핸들 자동 정리
	if (GrantedHandleMap.Contains(SourceTag))
	{
		UE_LOG(LogPBAbilitySystem, Warning,
			TEXT("GrantAbilitiesFromData: SourceTag [%s] 중복 Grant 감지, 기존 핸들 정리 후 재부여합니다."),
			*SourceTag.ToString());
		RemoveAbilitiesBySource(SourceTag);
	}

	// 실제 부여 로직은 DA에 위임, 결과 핸들만 저장
	GrantedHandleMap.Add(SourceTag, Data->GrantToAbilitySystem(this, CharacterLevel));

	const FPBAbilityGrantedHandles& Handles = GrantedHandleMap[SourceTag];
	UE_LOG(LogPBAbilitySystem, Log,
		TEXT("어빌리티 부여 완료: Source [%s] — 어빌리티 %d개, GE %d개"),
		*SourceTag.ToString(), Handles.AbilityHandles.Num(), Handles.EffectHandles.Num());
}

void UPBAbilitySystemComponent::RemoveAbilitiesBySource(const FGameplayTag& SourceTag)
{
	FPBAbilityGrantedHandles* Handles = GrantedHandleMap.Find(SourceTag);
	if (!Handles)
	{
		UE_LOG(LogPBAbilitySystem, Verbose,
			TEXT("RemoveAbilitiesBySource: 제거할 핸들 없음. (Source: %s)"),
			*SourceTag.ToString());
		return;
	}

	const int32 AbilityCount = Handles->AbilityHandles.Num();
	const int32 EffectCount = Handles->EffectHandles.Num();

	for (const FGameplayAbilitySpecHandle& Handle : Handles->AbilityHandles)
	{
		ClearAbility(Handle);
	}

	for (const FActiveGameplayEffectHandle& Handle : Handles->EffectHandles)
	{
		RemoveActiveGameplayEffect(Handle);
	}

	GrantedHandleMap.Remove(SourceTag);

	UE_LOG(LogPBAbilitySystem, Log,
		TEXT("어빌리티 제거 완료: Source [%s] — 어빌리티 %d개, GE %d개"),
		*SourceTag.ToString(), AbilityCount, EffectCount);
}

void UPBAbilitySystemComponent::RemoveAllSourceGrantedAbilities()
{
	// 순회 중 Remove 방지를 위해 키 복사
	TArray<FGameplayTag> SourceTags;
	GrantedHandleMap.GetKeys(SourceTags);

	for (const FGameplayTag& SourceTag : SourceTags)
	{
		RemoveAbilitiesBySource(SourceTag);
	}

	UE_LOG(LogPBAbilitySystem, Log,
		TEXT("전체 Source 어빌리티 제거 완료. (제거된 Source 수: %d)"),
		SourceTags.Num());
}

bool UPBAbilitySystemComponent::HasAbilitiesFromSource(const FGameplayTag& SourceTag) const
{
	return GrantedHandleMap.Contains(SourceTag);
}

TArray<FGameplayAbilitySpecHandle> UPBAbilitySystemComponent::GetAbilitySpecHandlesByTagFilter(
	const FGameplayTagContainer& RequireTags,
	const FGameplayTagContainer& IgnoreTags) const
{
	TArray<FGameplayAbilitySpecHandle> Result;

	// GetActivatableAbilities()는 non-const이므로 const_cast 사용
	// (Spec 데이터 자체는 수정하지 않음)
	const TArray<FGameplayAbilitySpec>& Specs =
		const_cast<UPBAbilitySystemComponent*>(this)->GetActivatableAbilities();

	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		// AssetTags + DynamicSpecSourceTags 합산
		FGameplayTagContainer CombinedTags;
		CombinedTags.AppendTags(Spec.Ability->GetAssetTags());
		CombinedTags.AppendTags(Spec.GetDynamicSpecSourceTags());

		// RequireTags를 모두 포함하는지 확인
		if (!CombinedTags.HasAll(RequireTags))
		{
			continue;
		}

		// IgnoreTags 중 하나라도 있으면 제외
		if (IgnoreTags.Num() > 0 && CombinedTags.HasAny(IgnoreTags))
		{
			continue;
		}

		Result.Add(Spec.Handle);
	}

	return Result;
}

void UPBAbilitySystemComponent::SetTurnAbilityActive(bool bActive)
{
	bIsTurnAbilityActive = bActive;
}

void UPBAbilitySystemComponent::ResetMovementResource()
{
	const float MaxMovement = GetNumericAttributeBase(UPBTurnResourceAttributeSet::GetMaxMovementAttribute());
	SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetMovementAttribute(), MaxMovement);
}

void UPBAbilitySystemComponent::OnProgressTurn()
{
	// 스택 차감 대상 핸들을 먼저 수집 (순회 중 컨테이너 변경 방지)
	TArray<FActiveGameplayEffectHandle> StackedHandles;
	for (const FActiveGameplayEffectHandle& ActiveGEHandle : ActiveGameplayEffects.GetAllActiveEffectHandles())
	{
		if (const UGameplayEffect* EffectCDO = GetGameplayEffectCDO(ActiveGEHandle))
		{
			if (EffectCDO->StackingType != EGameplayEffectStackingType::None)
			{
				StackedHandles.Add(ActiveGEHandle);
			}
		}
	}

	// 턴 기반 효과 적용 후 스택 차감
	for (const FActiveGameplayEffectHandle& ActiveGEHandle : StackedHandles)
	{
		const UGameplayEffect* EffectCDO = GetGameplayEffectCDO(ActiveGEHandle);
		if (!EffectCDO)
		{
			continue;
		}
		
		// PBTurnEffectComponent가 있으면 등록된 GE들을 적용 (원본 Spec의 SetByCaller 등 데이터 유지)
		if (const UPBTurnEffectComponent* TurnEffectComp = EffectCDO->FindComponent<UPBTurnEffectComponent>())
		{
			const FActiveGameplayEffect* ActiveGE = ActiveGameplayEffects.GetActiveGameplayEffect(ActiveGEHandle);
			if (ActiveGE)
			{
				for (const TSubclassOf<UGameplayEffect>& EffectClass : TurnEffectComp->GetTurnEffects())
				{
					if (const UGameplayEffect* TurnEffectCDO = EffectClass.GetDefaultObject())
					{
						FGameplayEffectSpec NewSpec;
						NewSpec.InitializeFromLinkedSpec(TurnEffectCDO, ActiveGE->Spec);
						ApplyGameplayEffectSpecToSelf(NewSpec);
					}
				}
			}
		}

		// 이펙트 스택 차감 (잔여 지속 턴 차감)
		RemoveActiveGameplayEffect(ActiveGEHandle, 1);
	}

	// 쿨다운 1턴씩 차감, 0 이하이면 제거
	TArray<FGameplayAbilitySpecHandle> ExpiredHandles;
	for (auto& Pair : CooldownMap)
	{
		Pair.Value -= 1;
		if (Pair.Value <= 0)
		{
			ExpiredHandles.Add(Pair.Key);
		}
	}
	for (const FGameplayAbilitySpecHandle& Handle : ExpiredHandles)
	{
		CooldownMap.Remove(Handle);
	}

	OnProgressTurnCompleted.Broadcast();
}

bool UPBAbilitySystemComponent::HasCooldown(const FGameplayAbilitySpecHandle& Handle) const
{
	const int32* Remaining = CooldownMap.Find(Handle);
	return Remaining != nullptr && *Remaining > 0;
}

void UPBAbilitySystemComponent::ApplyCooldown(const FGameplayAbilitySpecHandle& Handle, int32 CooldownTurns)
{
	if (CooldownTurns <= 0)
	{
		return;
	}
	CooldownMap.Add(Handle, CooldownTurns);
}

int32 UPBAbilitySystemComponent::GetRemainingCooldown(const FGameplayAbilitySpecHandle& Handle) const
{
	const int32* Remaining = CooldownMap.Find(Handle);
	return Remaining ? *Remaining : 0;
}

void UPBAbilitySystemComponent::NotifyGEExecuted(const FGameplayEffectSpec& Spec, const FGameplayAttribute& Attribute, float EffectiveValue)
{
	OnGEExecuted.Broadcast(Spec, Attribute, EffectiveValue);
}

void UPBAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPBAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void UPBAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
	Super::OnTagUpdated(Tag, TagExists);
	
	if (OnGameplayTagUpdated.IsBound())
	{
		OnGameplayTagUpdated.Broadcast(Tag, TagExists);
	}
}

void UPBAbilitySystemComponent::HandleActiveTurnChanged(AActor* CurrentTurnActor, int32 TurnIndex)
{
	if (CurrentTurnActor != GetOwner())
	{
		return;
	}
	
	// Owner의 턴이 되면 이펙트 스택 차감
	OnProgressTurn();
}

bool UPBAbilitySystemComponent::CanActivateAbilityByHandle(const FGameplayAbilitySpecHandle& Handle) const
{
	// GetActivatableAbilities는 non-const이므로 const_cast 사용
	const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	if (Spec == nullptr || !IsValid(Spec->Ability))
	{
		return false;
	}

	return Spec->Ability->CanActivateAbility(Handle, AbilityActorInfo.Get());
}
