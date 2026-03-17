// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "PBTurnEffectComponent.generated.h"

/**
 * 턴 기반 주기적 효과 GE 컴포넌트.
 * OnProgressTurn에서 이 컴포넌트를 가진 GE를 감지하면
 * TurnEffects에 등록된 GE들을 대상에게 적용한다.
 */
UCLASS(DisplayName = "Turn Effect", CollapseCategories, EditInlineNew)
class PROJECTB3_API UPBTurnEffectComponent : public UGameplayEffectComponent
{
	GENERATED_BODY()

public:
	/*~ UGameplayEffectComponent Interface ~*/
	virtual void OnGameplayEffectApplied(
		FActiveGameplayEffectsContainer& ActiveGEContainer,
		FGameplayEffectSpec& GESpec,
		FPredictionKey& PredictionKey) const override;

	/*~ UPBTurnEffectComponent Interface ~*/
	// 턴 효과 GE 목록 반환
	const TArray<TSubclassOf<UGameplayEffect>>& GetTurnEffects() const { return TurnEffects; }

protected:
	// 최초 적용 시 즉시 발동 여부 (false면 다음 턴부터 발동)
	UPROPERTY(EditDefaultsOnly, Category = "Turn Effect")
	bool bApplyOnInitialApplication = false;

	// 매 턴 적용할 GE 목록 (Instant 권장)
	UPROPERTY(EditDefaultsOnly, Category = "Turn Effect")
	TArray<TSubclassOf<UGameplayEffect>> TurnEffects;
};
