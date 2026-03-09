// PBInitializeTurnTask.h
#pragma once

#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "CoreMinimal.h"
#include "PBInitializeTurnTask.generated.h"

// 턴 진입 시 최초 1회만 자원(AP, 이동력)을 최대로 회복시키는 태스크
UCLASS(Blueprintable,
	meta = (DisplayName = "Initialize AI Turn Task", Category = "AI|Turn"))
class PROJECTB3_API UPBInitializeTurnTask : public UStateTreeTaskBlueprintBase
{
	GENERATED_BODY()

public:
	/*~ 입력 (Input) 바인딩 핀 ~*/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<AActor> SelfActor = nullptr;

protected:
	// StateTree가 이 테스크 상태로 진입할 때 1회 호출
	virtual EStateTreeRunStatus
	EnterState(FStateTreeExecutionContext& Context,
	           const FStateTreeTransitionResult& Transition) override;
};
