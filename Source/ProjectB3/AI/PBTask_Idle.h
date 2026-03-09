// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "CoreMinimal.h"
#include "PBTask_Idle.generated.h"


/**
 * 아무 행동도 하지 않고 무한히 대기(Running) 상태를 유지하는 범용 Task.
 * 턴 구조체 트리에서 특정 이벤트를 기다리기 위한 '휴식처' 용도로 사용됩니다.
 */
UCLASS(Blueprintable, meta = (DisplayName = "PB Task: Wait For Event (Idle)",
	Category = "Logic"))
class PROJECTB3_API UPBTask_Idle : public UStateTreeTaskBlueprintBase
{
	GENERATED_BODY()

public:
	UPBTask_Idle(const FObjectInitializer& ObjectInitializer);

protected:
	virtual EStateTreeRunStatus
	EnterState(FStateTreeExecutionContext& Context,
	           const FStateTreeTransitionResult& Transition) override;

	// 외부 이벤트 발생 등으로 중단(Interrupt)될 때 호출됩니다.
	virtual void ExitState(FStateTreeExecutionContext& Context,
	                       const FStateTreeTransitionResult&
	                       Transition) override;
};
