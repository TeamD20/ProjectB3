// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "PBEC_Heal.generated.h"

/**
 * 힐 ExecCalc.
 * 어빌리티에서 SetByCaller로 전달된 주사위 결과를 CalcFinalHeal로 보정한 뒤
 * IncomingHeal 메타 어트리뷰트에 출력한다.
 * PostGameplayEffectExecute에서 IncomingHeal → HP 회복이 처리된다.
 */
UCLASS()
class PROJECTB3_API UPBEC_Heal : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UPBEC_Heal();

	/*~ UGameplayEffectExecutionCalculation Interface ~*/
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput)
	const override;
};
