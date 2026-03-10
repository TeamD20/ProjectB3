// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility.h"
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