// PBAITypes.h
#pragma once

#include "CoreMinimal.h"
#include "PBAITypes.generated.h"
#include "UObject/NoExportTypes.h"


// 단일 행동의 종류를 정의하는 열거형
UENUM(BlueprintType)
enum class EPBActionType : uint8 { None, Move, Attack, UseItem };

// 행동에 필요한 코스트 데이터
USTRUCT(BlueprintType)
struct FPBCostData {
  GENERATED_BODY()

  // 소모할 행동
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  float ActionCost = 0.0f;

  // 소모할 보조 행동
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

// 조합 점수를 관리하고, 단일 행동(Single Action) 결과를 담는 객체
UCLASS(BlueprintType, Blueprintable)
class PROJECTB3_API UPBActionSequence : public UObject {
  GENERATED_BODY()

public:
  // 결정된 행동이 아무것도 없는지(턴 종료 상황인지) 확인
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Sequence")
  bool IsEmpty() const {
    return SingleAction.ActionType == EPBActionType::None;
  }

public:
  // 이 턴 행동 조합(Combo)이 갖는 최종 유틸리티 결산 점수
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  float TotalUtilityScore = 0.0f;
  /*~ 단일 행동 데이터 제공 ~*/

  // 결정된 단 1개의 행동 데이터
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI|Sequence")
  FPBSequenceAction SingleAction;
};
