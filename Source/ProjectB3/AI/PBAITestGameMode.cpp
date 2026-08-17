// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 최승현 (AI 전투 테스트 환경 작성)

// PBAITestGameMode.cpp

#include "PBAITestGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectB3/Characters/PBEnemyCharacter.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"


DEFINE_LOG_CATEGORY_STATIC(LogPBAITest, Log, All);

void APBAITestGameMode::BeginPlay() {
  Super::BeginPlay();

  if (bAutoStart) {
    // 캐릭터 초기화 대기 후 자동 전투 시작
    GetWorldTimerManager().SetTimer(AutoStartTimerHandle, this,
                                    &APBAITestGameMode::AutoStartTestCombat,
                                    AutoStartDelay, false);

    UE_LOG(LogPBAITest, Display,
           TEXT("[AITestGameMode] %.1f초 후 자동 전투 시작 예정."),
           AutoStartDelay);
  }
}

void APBAITestGameMode::AutoStartTestCombat() {
  UWorld *World = GetWorld();
  if (!IsValid(World)) {
    return;
  }

  UPBCombatManagerSubsystem *CombatManager =
      World->GetSubsystem<UPBCombatManagerSubsystem>();
  if (!IsValid(CombatManager) || CombatManager->IsInCombat()) {
    UE_LOG(LogPBAITest, Warning,
           TEXT("[AITestGameMode] CombatManager 없음 또는 이미 전투 중."));
    return;
  }

  TArray<AActor *> Combatants;

  // 1. 레벨에 배치된 모든 PBEnemyCharacter 수집 (적 AI)
  TArray<AActor *> FoundAI;
  UGameplayStatics::GetAllActorsOfClass(
      World, APBEnemyCharacter::StaticClass(), FoundAI);

  for (AActor *AI : FoundAI) {
    if (IsValid(AI) && Cast<IPBCombatParticipant>(AI)) {
      Combatants.Add(AI);
      UE_LOG(LogPBAITest, Display, TEXT("[AITestGameMode] AI 참가자 등록: %s"),
             *AI->GetName());
    }
  }

  // 2. Player Pawn 수집
  if (APlayerController *PC = World->GetFirstPlayerController()) {
    if (APawn *PlayerPawn = PC->GetPawn()) {
      if (Cast<IPBCombatParticipant>(PlayerPawn)) {
        Combatants.Add(PlayerPawn);
        UE_LOG(LogPBAITest, Display,
               TEXT("[AITestGameMode] 플레이어 참가자 등록: %s"),
               *PlayerPawn->GetName());
      } else {
        UE_LOG(LogPBAITest, Warning,
               TEXT("[AITestGameMode] Player Pawn이 "
                    "IPBCombatParticipant를 미구현. 건너뜀."));
      }
    }
  }

  // 3. 최소 2명 이상이면 전투 시작
  if (Combatants.Num() < 2) {
    UE_LOG(LogPBAITest, Warning,
           TEXT("[AITestGameMode] 전투 참가자가 %d명으로 부족. (최소 2명)"),
           Combatants.Num());
    return;
  }

  UE_LOG(LogPBAITest, Display,
         TEXT("[AITestGameMode] ===== 전투 시작! 총 %d명 참가 ====="),
         Combatants.Num());

  // 부모 클래스의 InitiateCombat으로 전투 개시
  InitiateCombat(Combatants);
}
