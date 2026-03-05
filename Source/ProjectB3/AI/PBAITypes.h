// PBAITypes.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PBAITypes.generated.h"


// 단일 행동의 종류를 정의하는 열거형
UENUM(BlueprintType)
enum class EPBActionType : uint8 { None, Move, Attack, UseItem };

// 행동에 필요한 코스트 데이터
USTRUCT(BlueprintType)
struct FPBCostData {
  GENERATED_BODY()

  // 소모할 행동 (발더스 3 식 Action)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  float ActionCost = 0.0f;

  // 소모할 보조 행동 (발더스 3 식 Bonus Action)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  float BonusActionCost = 0.0f;

  // 소모할 이동 거리
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  float MovementCost = 0.0f;
};

// 정규화된(0.0 ~ 1.0) 수치 모음 컨텍스트
USTRUCT(BlueprintType)
struct FPBCombatContext {
  GENERATED_BODY()

  // 타겟과의 거리 적합도 (거리가 알맞을수록 1.0)
  UPROPERTY(BlueprintReadWrite, Category = "AI|Utility")
  float NormalizedDistance = 0.0f;

  // 타겟의 취약성 점수 (HP & 명중률 계산 )
  UPROPERTY(BlueprintReadWrite, Category = "AI|Utility")
  float TargetVulnerability = 0.0f;

  // 고지대 이점 수치 (Z 높이 비교 및 Block 확인)
  UPROPERTY(BlueprintReadWrite, Category = "AI|Utility")
  float HighGroundAdvantage = 0.0f;
};

// 큐에 들어갈 단일 행동 객체
USTRUCT(BlueprintType)
struct FPBSequenceAction {
  GENERATED_BODY()

  // 어떤 행동을 할 것인가
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  EPBActionType ActionType = EPBActionType::None;

  // 이 행동의 타겟 (해당 액터를 목표로 이동, 공격 등 수행)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  TObjectPtr<AActor> TargetActor = nullptr;

  // 행동 발생 코스트
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  FPBCostData Cost;
};

// 조합 점수를 관리하고, 큐 방식으로 행동을 순차 실행하는 객체
UCLASS(BlueprintType, Blueprintable)
class UPBActionSequence : public UObject {
  GENERATED_BODY()

public:
  /*~ 큐 조작 함수 ~*/

  // 큐에 새로운 행동을 추가한다.
  UFUNCTION(BlueprintCallable, Category = "AI|Sequence")
  void EnqueueAction(const FPBSequenceAction &Action) {
    ActionQueue.Add(Action);
  }

  // 다음 행동을 뽑아낸다 (성공 시 true 반환).
  UFUNCTION(BlueprintCallable, Category = "AI|Sequence")
  bool DequeueAction(FPBSequenceAction &OutAction) {
    if (IsEmpty()) {
      return false;
    }

    OutAction = ActionQueue[0];
    ActionQueue.RemoveAt(0);
    return true;
  }

  // 대기 중인 콤보 작업이 비었는지 확인한다.
  UFUNCTION(BlueprintPure, Category = "AI|Sequence")
  bool IsEmpty() const { return ActionQueue.IsEmpty(); }

public:
  // 이 턴 행동 조합(Combo)이 갖는 최종 유틸리티 결산 점수
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  float TotalUtilityScore = 0.0f;

protected:
  // 행동들을 보관하는 배열 (이동 -> 행동 -> 추가행동 등 순차 적재)
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Sequence")
  TArray<FPBSequenceAction> ActionQueue;
};
