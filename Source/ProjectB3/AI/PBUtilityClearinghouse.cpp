#include "PBUtilityClearinghouse.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PBAIMockCharacter.h"
#include "PBGE_RestoreTurnResources.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"

#include "Kismet/GameplayStatics.h"
#include "PBAIMockAttributeSet.h"
#include "PBAIMockCharacter.h"
#include "PBGE_RestoreTurnResources.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"

// 임시 로깅을 위한 로그 카테고리 정의
DEFINE_LOG_CATEGORY_STATIC(LogPBUtility, Log, All);

/*~ 정규화 데이터 제공 인터페이스 ~*/

float UPBUtilityClearinghouse::GetNormalizedDistanceToTarget(
    AActor *TargetActor) const {
  // 유효성 검증
  if (!IsValid(TargetActor)) {
    UE_LOG(
        LogPBUtility, Warning,
        TEXT(
            "GetNormalizedDistanceToTarget: TargetActor가 유효하지 않습니다."));
    return 0.0f;
  }

  if (!IsValid(ActiveTurnActor)) {
    UE_LOG(LogPBUtility, Warning,
           TEXT("GetNormalizedDistanceToTarget: ActiveTurnActor가 설정되지 "
                "않았습니다."));
    return 0.0f;
  }

  // 캐싱 데이터 확인 (중복 연산 방지)
  if (const float *CachedValue = CachedDistanceMap.Find(TargetActor)) {
    return *CachedValue;
  }

  // TODO: 실제 맵(NavMesh) 스플라인 기반 경로 길이를 기반으로 0.0 ~ 1.0으로
  // 정규화 하는 로직

  float DummyDistanceScore = 0.9f;

  UE_LOG(
      LogPBUtility, Log,
      TEXT("AI [%s]가 타겟 [%s]과의 '거리 정규화 점수'를 요청함 -> 반환값: %f"),
      *ActiveTurnActor->GetName(), *TargetActor->GetName(), DummyDistanceScore);

  return DummyDistanceScore;
}

float UPBUtilityClearinghouse::GetTargetVulnerabilityScore(
    AActor *TargetActor) const {
  if (!IsValid(TargetActor)) {
    return 0.0f;
  }

  // 캐싱 데이터 우선 반환
  if (const float *CachedValue = CachedVulnerabilityMap.Find(TargetActor)) {
    return *CachedValue;
  }

  float CalculatedVulnerabilityScore = 0.8f; // 기본 더미값
  // TODO: Health AttributeSet이 추가되면 실제 HP 기반 취약성 계산으로 교체
  // 현재 UPBTurnResourceAttributeSet에는 Health가 없으므로 기본값 반환

  UE_LOG(LogPBUtility, Log,
         TEXT("AI [%s]가 타겟 [%s]의 '취약성 점수'를 요청함 -> 반환값: %f "
              "(기본 더미값)"),
         *(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
         *TargetActor->GetName(), CalculatedVulnerabilityScore);

  return CalculatedVulnerabilityScore;
}

float UPBUtilityClearinghouse::EvaluateHighGroundAdvantage(
    AActor *TargetActor) const {
  if (!IsValid(TargetActor) || !IsValid(ActiveTurnActor)) {
    return 0.0f;
  }

  // TODO: 트레이스(Raycast)를 통한 고저차 판별 및 엄폐물 여부 확인

  float DummyHighGroundScore = 0.4f;

  UE_LOG(LogPBUtility, Log,
         TEXT("AI [%s]가 타겟 [%s]에 대한 '고지대/엄폐 이점'을 요청함 -> "
              "반환값: %f"),
         *ActiveTurnActor->GetName(), *TargetActor->GetName(),
         DummyHighGroundScore);

  return DummyHighGroundScore;
}

/*~ 캐싱 (라이프사이클) 관리 인터페이스 ~*/

void UPBUtilityClearinghouse::CacheTurnData(AActor *CurrentTurnActor) {
  if (!IsValid(CurrentTurnActor)) {
    UE_LOG(LogPBUtility, Error,
           TEXT("CacheTurnData: CurrentTurnActor가 유효하지 않습니다."));
    return;
  }

  // 이전 캐시를 초기화한다.
  ActiveTurnActor = CurrentTurnActor;
  ClearCache();

  UE_LOG(LogPBUtility, Display,
         TEXT("=== AI [%s]의 턴 시작. 클리어링하우스 캐싱 작업 개시 ==="),
         *ActiveTurnActor->GetName());

  // 반경 N미터 내의 적대적 액터들을 긁어모아(Overlap 등), 거리 및 취약성
  // 점수를 미리 계산하여 TMap에 적재하는 로직 구현.
  // 현 단계에서는 "Player" 태그를 가진 모든 액터를 타겟으로 간주하여 캐시에
  // 넣습니다.
  TArray<AActor *> FoundPlayers;
  UGameplayStatics::GetAllActorsWithTag(CurrentTurnActor->GetWorld(),
                                        FName("Player"), FoundPlayers);

  for (AActor *PlayerActor : FoundPlayers) {
    if (IsValid(PlayerActor)) {
      CachedTargets.Add(PlayerActor);

      // 임시로 가변적인 점수를 부여하기 위해 고정값 대신 난수나 거리를 사용할
      // 수 있습니다. 여기서 우선 타겟으로 등록만 해둡니다.
    }
  }

  UE_LOG(LogPBUtility, Log,
         TEXT("주변 타겟 탐색 완료. 총 %d명의 잠재적 타겟 목록 캐싱 완료."),
         FoundPlayers.Num());
}

void UPBUtilityClearinghouse::ClearCache() {
  // 맵 컨테이너 요소들을 모두 비워 다음 턴이나 다른 캐릭터 연산 시 간섭이
  // 없도록 한다.
  CachedTargets.Empty();
  CachedDistanceMap.Empty();
  CachedVulnerabilityMap.Empty();
  CachedActionScoreMap.Empty();

  /*~ 스코어링 (ActionScore 산출) ~*/

  FPBTargetScore UPBUtilityClearinghouse::EvaluateActionScore(AActor *
                                                              TargetActor) {
    if (!IsValid(TargetActor)) {
      return FPBTargetScore{};
    }

    // 캐시 먼저 확인 (턴 내 중복 연산 방지)
    if (const FPBTargetScore *Cached = CachedActionScoreMap.Find(TargetActor)) {
      return *Cached;
    }

    FPBTargetScore Score;
    Score.TargetActor = TargetActor;

    // --- HitProbability 산정 ---
    // AI_System.md §7.1: (d20 + 공격보정 - 대상 AC) / 20, 클램프 [0.05, 0.95]
    // TODO: AI AttackModifier(GAS AS) 및 대상 ArmorClass(GAS AS) 연동 후 실값
    // 교체
    Score.HitProbability = FMath::Clamp(0.65f, 0.05f, 0.95f);

    // --- VulnerabilityWeight 산정 ---
    // AI_System.md §7.1 TargetModifiers: HP 비율 기반
    // GetTargetVulnerabilityScore가 Health AS 연동 후 실값 제공
    Score.VulnerabilityWeight = GetTargetVulnerabilityScore(TargetActor);

    // --- ArchetypeWeight 산정 ---
    // AI_System.md §4.4 Archetype.ScoreWeights.AttackWeight
    // TODO: Archetype 데이터 에셋 또는 UCurveFloat 연동 후 실값 교체
    Score.ArchetypeWeight = 1.0f;

    const float FinalScore = Score.GetActionScore();

    UE_LOG(LogPBUtility, Log,
           TEXT("[Scoring] AI [%s] → 타겟 [%s]: "
                "HitProb=%.2f, Vuln=%.2f, Archetype=%.2f → ActionScore=%.4f"),
           *(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
           *TargetActor->GetName(), Score.HitProbability,
           Score.VulnerabilityWeight, Score.ArchetypeWeight, FinalScore);

    // 결과 캐싱
    CachedActionScoreMap.Add(TargetActor, Score);
    return Score;
  }

  AActor *UPBUtilityClearinghouse::GetBestActionScoreTarget() {
    if (CachedTargets.IsEmpty()) {
      UE_LOG(LogPBUtility, Warning,
             TEXT("GetBestActionScoreTarget: CachedTargets가 비어 있습니다."));
      return nullptr;
    }

    AActor *BestTarget = nullptr;
    float BestScore = -1.0f;

    UE_LOG(LogPBUtility, Display,
           TEXT("=== [Scoring] 타겟 ActionScore 평가 개시 (%d명) ==="),
           CachedTargets.Num());

    for (const TWeakObjectPtr<AActor> &WeakTarget : CachedTargets) {
      if (!WeakTarget.IsValid()) {
        continue;
      }

      AActor *Target = WeakTarget.Get();
      const FPBTargetScore Score = EvaluateActionScore(Target);
      const float ActionScore = Score.GetActionScore();

      if (ActionScore > BestScore) {
        BestScore = ActionScore;
        BestTarget = Target;
      }
    }

    if (IsValid(BestTarget)) {
      UE_LOG(LogPBUtility, Display,
             TEXT("[Scoring] 최적 타겟 선정: [%s] (ActionScore=%.4f)"),
             *BestTarget->GetName(), BestScore);
    }

    return BestTarget;
  }

  /*~ 스코어링 (ActionScore 산출) ~*/

  FPBTargetScore UPBUtilityClearinghouse::EvaluateActionScore(AActor *
                                                              TargetActor) {
    if (!IsValid(TargetActor)) {
      return FPBTargetScore{};
    }

    // 캐시 먼저 확인 (턴 내 중복 연산 방지)
    if (const FPBTargetScore *Cached = CachedActionScoreMap.Find(TargetActor)) {
      return *Cached;
    }

    FPBTargetScore Score;
    Score.TargetActor = TargetActor;

    // --- HitProbability 산정 ---
    // AI_System.md §7.1: (d20 + 공격보정 - 대상 AC) / 20, 클램프 [0.05, 0.95]
    // TODO: AI AttackModifier(GAS AS) 및 대상 ArmorClass(GAS AS) 연동 후 실값
    // 교체
    Score.HitProbability = FMath::Clamp(0.65f, 0.05f, 0.95f);

    // --- VulnerabilityWeight 산정 ---
    // AI_System.md §7.1 TargetModifiers: HP 비율 기반
    // GetTargetVulnerabilityScore가 Health AS 연동 후 실값 제공
    Score.VulnerabilityWeight = GetTargetVulnerabilityScore(TargetActor);

    // --- ArchetypeWeight 산정 ---
    // AI_System.md §4.4 Archetype.ScoreWeights.AttackWeight
    // TODO: Archetype 데이터 에셋 또는 UCurveFloat 연동 후 실값 교체
    Score.ArchetypeWeight = 1.0f;

    // --- MovementScore 산정 ---
    // AI_System.md §7.2 PositionScore 경량화 버전
    // 공식: 1.0 - (DistToTarget / MaxMovementRange), 클램프 [0.0, 1.0]
    // 거리가 가까울수록 1.0, 몜수록 0.0
    // TODO: 이동력 AttributeSet 연동 후 Movement 실값 사용
    constexpr float MaxMovementRange = 1000.0f; // 임시 Default(10m)
    if (IsValid(ActiveTurnActor)) {
      const float DistToTarget = FVector::Dist(
          ActiveTurnActor->GetActorLocation(), TargetActor->GetActorLocation());
      Score.MovementScore =
          FMath::Clamp(1.0f - (DistToTarget / MaxMovementRange), 0.0f, 1.0f);
    }

    const float FinalScore = Score.GetTotalScore();

    UE_LOG(LogPBUtility, Log,
           TEXT("[Scoring] AI [%s] → 타겟 [%s]: "
                "HitProb=%.2f, Vuln=%.2f, Archetype=%.2f, Move=%.2f → "
                "TotalScore=%.4f"),
           *(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
           *TargetActor->GetName(), Score.HitProbability,
           Score.VulnerabilityWeight, Score.ArchetypeWeight,
           Score.MovementScore, FinalScore);

    // 결과 캐싱
    CachedActionScoreMap.Add(TargetActor, Score);
    return Score;
  }

  AActor *UPBUtilityClearinghouse::GetBestActionScoreTarget() {
    // GetTopKTargets(1) 위임 — 중복 루프 제거, TotalScore 기준 통일
    const TArray<FPBTargetScore> Top = GetTopKTargets(1);
    if (Top.Num() > 0 && IsValid(Top[0].TargetActor)) {
      return Top[0].TargetActor.Get();
    }
    return nullptr;
  }

  TArray<FPBTargetScore> UPBUtilityClearinghouse::GetTopKTargets(int32 K) {
    TArray<FPBTargetScore> AllScores;

    for (const TWeakObjectPtr<AActor> &WeakTarget : CachedTargets) {
      if (!WeakTarget.IsValid()) {
        continue;
      }
      AllScores.Add(EvaluateActionScore(WeakTarget.Get()));
    }

    // TotalScore 내림차순 정렬 (Optimization §4.2 Top-K 필터링)
    AllScores.Sort([](const FPBTargetScore &A, const FPBTargetScore &B) {
      return A.GetTotalScore() > B.GetTotalScore();
    });

    // 상위 K개 슬라이싱
    const int32 ResultCount = FMath::Min(K, AllScores.Num());

    UE_LOG(LogPBUtility, Display,
           TEXT("[TopK] CachedTargets %d명 중 상위 %d명 선정:"),
           AllScores.Num(), ResultCount);

    for (int32 i = 0; i < ResultCount; ++i) {
      UE_LOG(LogPBUtility, Display,
             TEXT("  [%d] %s → TotalScore=%.4f (Action=%.4f, Move=%.4f)"),
             i + 1,
             IsValid(AllScores[i].TargetActor)
                 ? *AllScores[i].TargetActor->GetName()
                 : TEXT("Invalid"),
             AllScores[i].GetTotalScore(), AllScores[i].GetActionScore(),
             AllScores[i].MovementScore);
    }

    TArray<FPBTargetScore> Result;
    for (int32 i = 0; i < ResultCount; ++i) {
      Result.Add(AllScores[i]);
    }
    return Result;
  }
