// PBGenerateSequenceTask.h
#pragma once

#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "CoreMinimal.h"
#include "PBAITypes.h"
#include "PBGenerateSequenceTask.generated.h"

// Clearinghouse에 질의하여 데이터를 수집하고, 최적의 행동 조합(Sequence)을 큐
// 형태로 생성하는 블루프린트 호환 테스크
UCLASS(Blueprintable, meta = (DisplayName = "Generate AI Action Sequence",
                              Category = "AI|Sequence"))
class PROJECTB3_API UPBGenerateSequenceTask
    : public UStateTreeTaskBlueprintBase {
  GENERATED_BODY()

public:
  /*~ 입력 (Input) 바인딩 핀 ~*/
  // StateTree에서 바인딩 받을 현재 턴을 수행 중인 AI 액터
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input")
  TObjectPtr<AActor> SelfActor = nullptr;

  /*~ 출력 (Output) 바인딩 핀 ~*/
  // 이 테스크가 뱉어낼 최종 콤보 시퀀스 결과물 데이터 (부모 방향으로 출력 배출
  // 허용을 위해 Output 카테고리 유지)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
  TObjectPtr<UPBActionSequence> GeneratedSequence = nullptr;

  /*~ UStateTreeTaskBlueprintBase Interface ~*/
protected:
  // StateTree가 이 테스크 상태로 진입할 때 1회 호출되는 메인 실행 로직
  virtual EStateTreeRunStatus
  EnterState(FStateTreeExecutionContext &Context,
             const FStateTreeTransitionResult &Transition) override;
};
