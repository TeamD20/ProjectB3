// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnResourceAttributeSet.h"
#include "GameplayEffectExtension.h"

UPBTurnResourceAttributeSet::UPBTurnResourceAttributeSet()
{
	InitAction(1.0f);
	InitBonusAction(1.0f);
	InitReaction(1.0f);
	InitMovement(30.0f);
}

void UPBTurnResourceAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 모든 턴 자원은 0 미만으로 내려갈 수 없다
	NewValue = FMath::Max(0.0f, NewValue);
}

void UPBTurnResourceAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 0 미만 클램프 (GE에 의한 직접 변경 시)
	if (Data.EvaluatedData.Attribute == GetActionAttribute())
	{
		SetAction(FMath::Max(0.0f, GetAction()));
	}
	else if (Data.EvaluatedData.Attribute == GetBonusActionAttribute())
	{
		SetBonusAction(FMath::Max(0.0f, GetBonusAction()));
	}
	else if (Data.EvaluatedData.Attribute == GetReactionAttribute())
	{
		SetReaction(FMath::Max(0.0f, GetReaction()));
	}
	else if (Data.EvaluatedData.Attribute == GetMovementAttribute())
	{
		SetMovement(FMath::Max(0.0f, GetMovement()));
	}
}
