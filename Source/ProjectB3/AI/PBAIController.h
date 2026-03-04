// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "CoreMinimal.h"
#include "PBAIController.generated.h"


class UStateTreeComponent;

UCLASS()
class PROJECTB3_API APBAIController : public AAIController {
  GENERATED_BODY()

  /*~ 생성자 ~*/
public:
  APBAIController();

  /*~ AController Interface ~*/
protected:
  virtual void OnPossess(APawn *InPawn) override;

  /*~ 컴포넌트 ~*/
protected:
  // AI 행동 트리를 구동하는 StateTree 컴포넌트
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|StateTree")
  TObjectPtr<UStateTreeComponent> StateTreeComponent;
};
