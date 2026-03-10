// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"

EPBAbilityType UPBGameplayAbility::GetAbilityType() const
{
	// 활성화 중이면 Spec의 DynamicAbilityTags도 포함하여 조회
	FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (Spec)
	{
		FGameplayTagContainer CombinedTags;
		CombinedTags.AppendTags(GetAssetTags());
		CombinedTags.AppendTags(Spec->GetDynamicSpecSourceTags());
		return GetAbilityTypeFromTags(CombinedTags);
	}

	// 비활성 상태면 CDO의 AbilityTags만 조회
	return GetAbilityTypeFromTags(GetAssetTags());
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