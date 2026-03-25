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

  // Generate의 EQS 좌표 최적화 완료 대기 여부
  // bIsReady가 false인 동안 행동 실행을 보류하고 Tick에서 폴링한다.
  UPROPERTY()
  bool bWaitingForSequenceReady = false;

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

  // EndMode=Manual 어빌리티에서 EndAbility 미호출 시 무한 대기 방지 타임아웃
  float AbilityTimeoutRemaining = 0.0f;
  static constexpr float AbilityTimeoutSeconds = 3.0f;

  // AdvanceToNextAction 이중 진입 방지 (타임아웃 + OnAbilityEnded 동시 호출 가드)
  bool bIsAdvancing = false;

  // AI 행동 간 시각적 간격을 위한 딜레이 (초). 발표/데모 시 조절 가능.
  static constexpr float ActionDelaySeconds = 0.8f;

  // 턴 시작 시각적 딜레이 (초). DoT/HoT 큐 연출 후 AI 행동 시작까지 대기.
  static constexpr float TurnStartDelaySeconds = 1.0f;

  // 턴 시작 딜레이 잔여 시간 (Tick에서 카운트다운, 첫 마이크로 턴에서만 적용)
  float TurnStartDelayRemaining = 0.f;

  // 딜레이 후 다음 행동 실행을 예약 (비동기 행동 완료 콜백에서 호출)
  void ScheduleNextAction();

  // 행동 간 딜레이 잔여 시간 (Tick에서 카운트다운)
  float ActionDelayRemaining = 0.f;

  // StateTree Input 바인딩은 매 Tick마다 소스(Generate)에서 재복사되어
  // CurrentActionIndex가 0으로 초기화된다. 실행 로직에서는 이 로컬 복사본을 사용하여
  // 바인딩 갱신으로부터 실행 상태를 격리한다.
  FPBActionSequence ExecutionSequence;

  // 사망한 타겟을 참조하는 잔여 행동을 일괄 무효화한다.
  // ActionType=None + Cost=0으로 교체하여 자원을 보존한다.
  void InvalidateActionsForDeadTarget(const AActor* DeadTarget);

  // 디버거용: 현재 실행 상태를 Clearinghouse에 캐싱
  void UpdateExecutionDebugState() const;
};
