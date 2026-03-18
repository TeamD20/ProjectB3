// PBAITestGameMode.h
// AI 턴 시스템 테스트 전용 GameMode.
// 레벨에 배치된 PBEnemyCharacter + Player를 자동 수집하여 전투를 개시한다.
#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/Game/PBGameplayGameMode.h"
#include "PBAITestGameMode.generated.h"


UCLASS()
class PROJECTB3_API APBAITestGameMode : public APBGameplayGameMode {
  GENERATED_BODY()

public:
  // 레벨의 모든 PBEnemyCharacter + Player Pawn을 자동 수집하여 전투 시작.
  // 레벨 BP BeginPlay에서 호출하거나, bAutoStart = true 시 자동 실행.
  UFUNCTION(BlueprintCallable, Category = "AI|Test")
  void AutoStartTestCombat();

protected:
  virtual void BeginPlay() override;

  // true면 BeginPlay에서 자동으로 전투 시작 (BP에서 토글 가능)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Test")
  bool bAutoStart = false;

  // 자동 시작 시 딜레이 (초). 캐릭터 초기화 대기용.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Test",
            meta = (EditCondition = "bAutoStart"))
  float AutoStartDelay = 1.0f;

private:
  FTimerHandle AutoStartTimerHandle;
};
