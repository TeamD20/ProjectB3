// PBUtilityClearinghouse.cpp

#include "PBUtilityClearinghouse.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "PBAIMockAttributeSet.h"
#include "PBAIMockCharacter.h"

#include "Kismet/GameplayStatics.h"
#include "PBAIMockAttributeSet.h"
#include "PBAIMockCharacter.h"
#include "PBGE_RestoreTurnResources.h"

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

  // 실제 HP 기반 취약성 계산
  if (APBAIMockCharacter *MockTarget = Cast<APBAIMockCharacter>(TargetActor)) {
    if (const UPBAIMockAttributeSet *AttrSet = MockTarget->GetAttributeSet()) {
      float CurrentHP = AttrSet->GetHealth();
      float MaxHP = 100.0f;
      // 방금 추가된 PBAIMockCharacter의 속성 사용

      if (MaxHP > 0.0f) {
        // HP가 낮을수록 취약성(Vulnerability) 점수는 높아짐 (0.0 ~ 1.0)
        CalculatedVulnerabilityScore =
            1.0f - FMath::Clamp(CurrentHP / MaxHP, 0.0f, 1.0f);
      }

      UE_LOG(LogPBUtility, Log,
             TEXT("AI [%s] evaluated Target [%s]: CurrentHP = %f, MaxHP = %f, "
                  "CalculatedScore = %f"),
             *(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
             *TargetActor->GetName(), CurrentHP, MaxHP,
             CalculatedVulnerabilityScore);
    }
  }

  UE_LOG(LogPBUtility, Log,
         TEXT("AI [%s]가 타겟 [%s]의 '취약성 점수'를 요청함 -> 반환값: %f "
              "(실제 HP 기반)"),
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

  UE_LOG(LogPBUtility, Log,
         TEXT("클리어링하우스 메모리 캐시가 성공적으로 비워졌습니다."));
}

void UPBUtilityClearinghouse::RestoreTurnResources(AActor *CurrentTurnActor) {
  if (!IsValid(CurrentTurnActor)) {
    return;
  }

  APBAIMockCharacter *MockChar = Cast<APBAIMockCharacter>(CurrentTurnActor);
  if (!MockChar)
    return;

  UAbilitySystemComponent *ASC = MockChar->GetAbilitySystemComponent();
  const UPBAIMockAttributeSet *AttrSet = MockChar->GetAttributeSet();

  if (ASC && AttrSet) {
    // GE_RestoreTurnResources 에셋을 생성하여 턴 시작 자원을 최대치로 초기화
    UGameplayEffect *RestoreGE = NewObject<UPBGE_RestoreTurnResources>(
        GetTransientPackage(), UPBGE_RestoreTurnResources::StaticClass());

    if (RestoreGE) {
      ASC->ApplyGameplayEffectToSelf(RestoreGE, 1.0f, ASC->MakeEffectContext());
    }

    UE_LOG(LogPBUtility, Display,
           TEXT(">> [Turn Restore] %s 의 턴 자원이 모두 회복되었습니다! "
                "(Action: %f, Movement: %f)"),
           *CurrentTurnActor->GetName(), AttrSet->GetMaxAction(),
           AttrSet->GetMaxMovement());
  }
}
