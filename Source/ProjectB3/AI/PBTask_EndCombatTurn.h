// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "CoreMinimal.h"
#include "PBTask_EndCombatTurn.generated.h"

/**
 * 마이크로 턴 루프(Micro-Turn Loop) 종료 시 호출되는 전용 Task.
 * 남은 자원으로 할 행동이 없다고 판단될 때 이 Task로 들어와,
 * 전투 매니저 서브시스템(PBCombatManagerSubsystem)에 턴 종료를 선언한다.
 */
UCLASS(Blueprintable,
	meta = (DisplayName = "End Combat Turn", Category = "AI|Combat"))
class PROJECTB3_API UPBTask_EndCombatTurn : public UStateTreeTaskBlueprintBase
{
	GENERATED_BODY()

public:
	/*~ 입력 (Input) 바인딩 핀 ~*/
	// StateTree에서 바인딩 받을 현재 턴을 수행 중인 액터
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<AActor> SelfActor = nullptr;

	/*~ UStateTreeTaskBlueprintBase Interface ~*/
protected:
	// StateTree가 이 Task 상태로 진입할 때 1회 호출되는 메인 실행 로직
	virtual EStateTreeRunStatus
	EnterState(FStateTreeExecutionContext& Context,
	           const FStateTreeTransitionResult& Transition) override;
};
