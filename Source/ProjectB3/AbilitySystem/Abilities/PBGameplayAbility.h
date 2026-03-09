// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBGameplayAbility.generated.h"

/** 프로젝트 전용 GameplayAbility 기반 클래스. 모든 어빌리티는 이 클래스를 상속해서 구현. */
UCLASS()
class PROJECTB3_API UPBGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	// GAS 내장 쿨다운 GE 미사용. 턴제 쿨다운은 별도 관리.
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;


protected:
	// Payload에서 타겟 데이터 추출 유틸리티
	FPBAbilityTargetData ExtractTargetDataFromEvent(const FGameplayEventData& EventData) const;

protected:
	// 자원 소모 유형 (CostGameplayEffectClass와 대응)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Cost")
	EPBAbilityCostType CostType = EPBAbilityCostType::Action;
	
};
