// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"

EPBAbilityType UPBGameplayAbility::GetAbilityType() const
{
	// CDO가 아닌 인스턴스에서만 Spec의 DynamicAbilityTags를 포함하여 조회한다.
	if (IsInstantiated())
	{
		if (FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
		{
			FGameplayTagContainer CombinedTags;
			CombinedTags.AppendTags(GetAssetTags());
			CombinedTags.AppendTags(Spec->GetDynamicSpecSourceTags());
			return GetAbilityTypeFromTags(CombinedTags);
		}
	}

	// CDO이거나 활성 Spec이 없으면 AssetTags만 조회한다.
	return GetAbilityTypeFromTags(GetAssetTags());
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
	if (GetAbilityType() == EPBAbilityType::None)
	{
		return true;
	}

	// 타입 지정 어빌리티가 이미 실행 중이면 활성화 거부
	if (const UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()))
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
	// 타입 지정 어빌리티인 경우 ASC에 실행 중 플래그 설정
	if (GetAbilityType() != EPBAbilityType::None)
	{
		if (UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()))
		{
			PBASC->SetTurnAbilityActive(true);
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPBGameplayAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 타입 지정 어빌리티인 경우 ASC 플래그 해제
	if (GetAbilityType() != EPBAbilityType::None)
	{
		if (UPBAbilitySystemComponent* PBASC = Cast<UPBAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()))
		{
			PBASC->SetTurnAbilityActive(false);
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


FPBAbilityTargetData UPBGameplayAbility::ExtractTargetDataFromEvent(
	const FGameplayEventData& EventData) const
{
	if (const UPBTargetPayload* Payload = Cast<UPBTargetPayload>(EventData.OptionalObject))
	{
		return Payload->TargetData;
	}

	return FPBAbilityTargetData();
}