// PBExecuteSequenceTask.h
#pragma once

#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "PBAITypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "PBExecuteSequenceTask.generated.h"

class UAbilitySystemComponent;
// GenerateSequenceTask가 만들어둔 콤보 데이터를 소비하는 블루프린트 호환 테스크
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

  // 부모 State 등으로부터 전달받을 행동 데이터 구조체
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
  FPBActionSequence SequenceToExecute;

  /* 내부 상태 (State) 변수들 - 에디터 노출 보이지 않게 처리해둠 */
  UPROPERTY()
  bool bIsActionInProgress = false;

  UPROPERTY()
  FPBSequenceAction CurrentAction;

  UPROPERTY()
  TObjectPtr<class AAIController> CachedAIController = nullptr;

  /*~ UStateTreeTaskBlueprintBase Interface ~*/
protected:
  virtual EStateTreeRunStatus
  EnterState(FStateTreeExecutionContext &Context,
             const FStateTreeTransitionResult &Transition) override;
  virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext &Context,
                                   const float DeltaTime) override;
  virtual void ExitState(FStateTreeExecutionContext &Context,
                         const FStateTreeTransitionResult &Transition) override;

private:
  // 현재 CurrentAction을 실행한다.
  // Running: 비동기 작업 시작 (콜백 대기)
  // Succeeded: 동기적 완료 (타겟 사망 스킵 등)
  // Failed: 실행 불가 (자원 부족, 어빌리티 없음 등)
  EStateTreeRunStatus ProcessSingleAction();

  // 콜백에서 호출: 이전 델리게이트 정리 후 다음 행동을 소비하여 실행.
  // 더 이상 행동이 없으면 bIsActionInProgress = false로 시퀀스 완료 처리.
  void AdvanceToNextAction();

  EStateTreeRunStatus UpdateCurrentAction(float DeltaTime);

protected:
  FGameplayAbilitySpecHandle ActiveSpecHandle;
  FDelegateHandle DelegateHandle;

  UPROPERTY()
  UAbilitySystemComponent *CachedASC = nullptr;
};
