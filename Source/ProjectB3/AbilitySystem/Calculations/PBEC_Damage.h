// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "PBEC_Damage.generated.h"

/**
 * 데미지 ExecCalc.
 * 어빌리티에서 SetByCaller로 전달된 주사위 결과와 공격 수정치를 합산하고,
 * 대상의 저항/취약을 적용한 뒤 IncomingDamage 메타 어트리뷰트에 출력한다.
 * PostGameplayEffectExecute에서 IncomingDamage → HP 차감이 처리된다.
 */
UCLASS()
class PROJECTB3_API UPBEC_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UPBEC_Damage();

	/*~ UGameplayEffectExecutionCalculation Interface ~*/
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
