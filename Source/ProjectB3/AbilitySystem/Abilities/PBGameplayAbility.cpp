// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"

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