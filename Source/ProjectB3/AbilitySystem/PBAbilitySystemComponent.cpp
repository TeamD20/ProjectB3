// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySystemComponent.h"
#include "Data/PBAbilitySetData.h"

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

	const FPBSourceGrantedHandles& Handles = GrantedHandleMap[SourceTag];
	UE_LOG(LogPBAbilitySystem, Log,
		TEXT("어빌리티 부여 완료: Source [%s] — 어빌리티 %d개, GE %d개"),
		*SourceTag.ToString(), Handles.AbilityHandles.Num(), Handles.EffectHandles.Num());
}

void UPBAbilitySystemComponent::RemoveAbilitiesBySource(const FGameplayTag& SourceTag)
{
	FPBSourceGrantedHandles* Handles = GrantedHandleMap.Find(SourceTag);
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
