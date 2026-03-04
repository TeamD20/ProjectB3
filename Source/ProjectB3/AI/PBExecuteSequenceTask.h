// PBExecuteSequenceTask.h
#pragma once

#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "CoreMinimal.h"
#include "PBAITypes.h"
#include "PBExecuteSequenceTask.generated.h"

// GenerateSequenceTask가 만들어둔 콤보(큐)를 넘겨받아 차례대로(Dequeue) 하나씩
// 실행시키는 블루프린트 호환 테스크
UCLASS(Blueprintable, meta = (DisplayName = "Execute AI Action Sequence",
                              Category = "AI|Sequence"))
class PROJECTB3_API UPBExecuteSequenceTask
    : public UStateTreeTaskBlueprintBase {
  GENERATED_BODY()

public:
  UPBExecuteSequenceTask(const FObjectInitializer &ObjectInitializer);

  /*~ 입력 (Input) 바인딩 핀 ~*/

  // 행동을 수행할 주체 액터
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  TObjectPtr<AActor> SelfActor = nullptr;

  // 이전 Task(GenerateSequenceTask)에서 전달받을 행동 콤보(Queue) 객체
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  TObjectPtr<UPBActionSequence> SequenceToExecute = nullptr;

  /*~ UStateTreeTaskBlueprintBase Interface ~*/
protected:
  // StateTree가 이 테스크 상태로 진입할 때 1회 호출되는 실행 로직
  virtual EStateTreeRunStatus
  EnterState(FStateTreeExecutionContext &Context,
             const FStateTreeTransitionResult &Transition) override;

  // 비동기 작업(이동 등) 대기를 위한 Tick 기능
  virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext &Context,
                                   const float DeltaTime) override;

  virtual void ExitState(FStateTreeExecutionContext &Context,
                         const FStateTreeTransitionResult &Transition) override;

protected:
  // 현재 처리를 진행 중인지 여부
  UPROPERTY()
  bool bIsActionInProgress = false;

  // 현재 처리 중인 액션
  UPROPERTY()
  FPBSequenceAction CurrentAction;

  // 로깅용 스텝 카운터
  UPROPERTY()
  int32 ActionStep = 1;

  // AI 컨트롤러 캐싱용
  UPROPERTY()
  TObjectPtr<class AAIController> CachedAIController = nullptr;

private:
  EStateTreeRunStatus ProcessNextAction();
  EStateTreeRunStatus UpdateCurrentAction(float DeltaTime);
};
