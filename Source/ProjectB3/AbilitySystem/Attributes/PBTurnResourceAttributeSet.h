// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AI/PBAIMockAttributeSet.h"
#include "PBTurnResourceAttributeSet.generated.h"

/**
 * 턴 자원 AttributeSet.
 */
UCLASS()
class PROJECTB3_API UPBTurnResourceAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPBTurnResourceAttributeSet();
	
	/*~ UAttributeSet Interface ~*/
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
public:
	// 주 행동 (기본 1)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|TurnResource")
	FGameplayAttributeData Action;
	ATTRIBUTE_ACCESSORS(UPBTurnResourceAttributeSet, Action)

	// 보조 행동
	UPROPERTY(BlueprintReadOnly, Category = "Combat|TurnResource")
	FGameplayAttributeData BonusAction;
	ATTRIBUTE_ACCESSORS(UPBTurnResourceAttributeSet, BonusAction)

	// 반응 행동 (라운드당)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|TurnResource")
	FGameplayAttributeData Reaction;
	ATTRIBUTE_ACCESSORS(UPBTurnResourceAttributeSet, Reaction)

	// 이동력 (cm 단위)
	UPROPERTY(BlueprintReadOnly, Category = "Combat|TurnResource")
	FGameplayAttributeData Movement;
	ATTRIBUTE_ACCESSORS(UPBTurnResourceAttributeSet, Movement)
};
