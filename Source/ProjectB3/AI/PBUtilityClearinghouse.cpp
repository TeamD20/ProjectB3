#include "PBUtilityClearinghouse.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "PBAIMockCharacter.h"
#include "PBGE_RestoreTurnResources.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"

// 임시 로깅을 위한 로그 카테고리 정의
DEFINE_LOG_CATEGORY_STATIC(LogPBUtility, Log, All);

/*~ 정규화 데이터 제공 인터페이스 ~*/

float UPBUtilityClearinghouse::GetNormalizedDistanceToTarget(
	AActor* TargetActor) const
{
	// 유효성 검증
	if (!IsValid(TargetActor))
	{
		UE_LOG(
			LogPBUtility, Warning,
			TEXT(
				"GetNormalizedDistanceToTarget: TargetActor가 유효하지 않습니다."));
		return 0.0f;
	}

	if (!IsValid(ActiveTurnActor))
	{
		UE_LOG(LogPBUtility, Warning,
		       TEXT("GetNormalizedDistanceToTarget: ActiveTurnActor가 설정되지 "
			       "않았습니다."));
		return 0.0f;
	}

	// 캐싱 데이터 확인 (중복 연산 방지)
	if (const float* CachedValue = CachedDistanceMap.Find(TargetActor))
	{
		return *CachedValue;
	}

	// TODO: 실제 맵(NavMesh) 스플라인 기반 경로 길이를 기반으로 0.0 ~ 1.0으로
	// 정규화 하는 로직

	float DummyDistanceScore = 0.9f;

	UE_LOG(
		LogPBUtility, Log,
		TEXT("AI [%s]가 타겟 [%s]과의 '거리 정규화 점수'를 요청함 -> 반환값: %f"),
		*ActiveTurnActor->GetName(), *TargetActor->GetName(),
		DummyDistanceScore);

	return DummyDistanceScore;
}

float UPBUtilityClearinghouse::GetTargetVulnerabilityScore(
	AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return 0.0f;
	}

	// 캐싱 데이터 우선 반환
	if (const float* CachedValue = CachedVulnerabilityMap.Find(TargetActor))
	{
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
	AActor* TargetActor) const
{
	if (!IsValid(TargetActor) || !IsValid(ActiveTurnActor))
	{
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

void UPBUtilityClearinghouse::RestoreTurnResources(AActor* CurrentTurnActor)
{
	if (!IsValid(CurrentTurnActor))
	{
		return;
	}

	APBAIMockCharacter* MockChar = Cast<APBAIMockCharacter>(CurrentTurnActor);
	if (!MockChar)
		return;

	UAbilitySystemComponent* ASC = MockChar->GetAbilitySystemComponent();
	const UPBTurnResourceAttributeSet* AttrSet = MockChar->
		GetTurnResourceAttributeSet();

	if (ASC && AttrSet)
	{
		// GE_RestoreTurnResources 에셋을 생성하여 턴 시작 자원을 최대치로 초기화
		UGameplayEffect* RestoreGE = NewObject<UPBGE_RestoreTurnResources>(
			GetTransientPackage(), UPBGE_RestoreTurnResources::StaticClass());

		if (RestoreGE)
		{
			ASC->ApplyGameplayEffectToSelf(RestoreGE, 1.0f,
			                               ASC->MakeEffectContext());
		}

		UE_LOG(LogPBUtility, Display,
		       TEXT(">> [Turn Restore] %s 의 턴 자원이 모두 회복되었습니다! "
			       "(Action: %f, Movement: %f)"),
		       *CurrentTurnActor->GetName(), AttrSet->GetAction(),
		       AttrSet->GetMovement());
	}
}

void UPBUtilityClearinghouse::CacheTurnData(AActor* CurrentTurnActor)
{
	if (!IsValid(CurrentTurnActor))
	{
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
	// 현 단계에서는 "Player" 태그를 가진 모든 액터를 타겟
	TArray<AActor*> FoundPlayers;
	UGameplayStatics::GetAllActorsWithTag(CurrentTurnActor->GetWorld(),
	                                      FName("Player"), FoundPlayers);

	for (AActor* PlayerActor : FoundPlayers)
	{
		if (IsValid(PlayerActor))
		{
			CachedTargets.Add(PlayerActor);

			// 임시로 가변적인 점수를 부여하기 위해 고정값 대신 난수나 거리를 사용가능
		}
	}

	UE_LOG(LogPBUtility, Log,
	       TEXT("주변 타겟 탐색 완료. 총 %d명의 잠재적 타겟 목록 캐싱 완료."),
	       FoundPlayers.Num());
}

void UPBUtilityClearinghouse::ClearCache()
{
	// 맵 컨테이너 요소들을 모두 비워 다음 턴이나 다른 캐릭터 연산 시 간섭이
	// 없도록 한다.
	CachedTargets.Empty();
	CachedDistanceMap.Empty();
	CachedVulnerabilityMap.Empty();
	CachedActionScoreMap.Empty();

	UE_LOG(LogPBUtility, Log,
	       TEXT("클리어링하우스 메모리 캐시가 성공적으로 비워졌습니다."));
}

/*~ 스코어링 (ActionScore 산출) ~*/

FPBTargetScore
UPBUtilityClearinghouse::EvaluateActionScore(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return FPBTargetScore{};
	}

	// 캐시 먼저 확인 (턴 내 중복 연산 방지)
	if (const FPBTargetScore* Cached = CachedActionScoreMap.Find(TargetActor))
	{
		return *Cached;
	}

	FPBTargetScore Score;
	Score.TargetActor = TargetActor;

	// --- ExpectedDamage 산정 ---
	// 명중 확률을 내포한 유효 기대 피해량
	// TODO: Phase 2에서 어빌리티의 GetExpectedXxxDamage() 호출로 교체
	// 현재 더미값: 기존 BaseScore(10.0) × HitProbability(0.65) 등가
	Score.ExpectedDamage = 6.5f;

	// --- TargetModifier 산정 ---
	// ThreatMultiplier × RoleMultiplier
	// 현재는 취약성 점수를 TargetModifier로 사용 (임시)
	// TODO: ThreatScore 시스템 연동 후 실값 교체
	Score.TargetModifier = GetTargetVulnerabilityScore(TargetActor);

	// --- SituationalBonus 산정 ---
	// TODO: 환경 상호작용, 처치 보너스, 집중 파괴 등
	Score.SituationalBonus = 0.0f;

	// --- ArchetypeWeight 산정 ---
	// TODO: Archetype 데이터 에셋 또는 UCurveFloat 연동 후 실값 교체
	Score.ArchetypeWeight = 1.0f;

	// --- MovementScore 산정 ---
	// 공식: 1.0 - (DistToTarget / MaxMovementRange), 클램프 [0.0, 1.0]
	// 거리가 가까울수록 1.0, 멀수록 0.0
	// TODO: 이동력 AttributeSet 연동 후 Movement 실값 사용
	constexpr float MaxMovementRange = 1000.0f; // 임시 Default(10m)
	if (IsValid(ActiveTurnActor))
	{
		const float DistToTarget = FVector::Dist(
			ActiveTurnActor->GetActorLocation(),
			TargetActor->GetActorLocation());
		Score.MovementScore =
			FMath::Clamp(1.0f - (DistToTarget / MaxMovementRange), 0.0f, 1.0f);
	}

	const float FinalScore = Score.GetTotalScore();

	UE_LOG(LogPBUtility, Log,
	       TEXT("[Scoring] AI [%s] → 타겟 [%s]: "
		       "ExpDmg=%.1f, TargetMod=%.2f, "
		       "Situational=%.1f, Archetype=%.2f, Move=%.2f → "
		       "TotalScore=%.4f"),
	       *(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
	       *TargetActor->GetName(), Score.ExpectedDamage,
	       Score.TargetModifier, Score.SituationalBonus,
	       Score.ArchetypeWeight, Score.MovementScore,
	       FinalScore);

	// 결과 캐싱
	CachedActionScoreMap.Add(TargetActor, Score);
	return Score;
}

AActor* UPBUtilityClearinghouse::GetBestActionScoreTarget()
{
	// GetTopKTargets(1) 위임 — 중복 루프 제거, TotalScore 기준 통일
	const TArray<FPBTargetScore> Top = GetTopKTargets(1);
	if (Top.Num() > 0 && IsValid(Top[0].TargetActor))
	{
		return Top[0].TargetActor.Get();
	}
	return nullptr;
}

TArray<FPBTargetScore> UPBUtilityClearinghouse::GetTopKTargets(int32 K)
{
	TArray<FPBTargetScore> AllScores;

	for (const TWeakObjectPtr<AActor>& WeakTarget : CachedTargets)
	{
		if (!WeakTarget.IsValid())
		{
			continue;
		}
		AllScores.Add(EvaluateActionScore(WeakTarget.Get()));
	}

	AllScores.Sort([](const FPBTargetScore& A, const FPBTargetScore& B)
	{
		return A.GetTotalScore() > B.GetTotalScore();
	});

	// 상위 K개 슬라이싱
	const int32 ResultCount = FMath::Min(K, AllScores.Num());

	UE_LOG(LogPBUtility, Display,
	       TEXT("[TopK] CachedTargets %d명 중 상위 %d명 선정:"), AllScores.Num(),
	       ResultCount);

	for (int32 i = 0; i < ResultCount; ++i)
	{
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
	for (int32 i = 0; i < ResultCount; ++i)
	{
		Result.Add(AllScores[i]);
	}
	return Result;
}

/*~ Fallback 위치 계산 ~*/

FVector UPBUtilityClearinghouse::CalculateFallbackPosition(
	AActor* SelfRef, float RemainingMP) const
{
	if (!IsValid(SelfRef) || CachedTargets.Num() == 0)
	{
		UE_LOG(LogPBUtility, Warning,
		       TEXT("[Fallback] Self 또는 CachedTargets가 유효하지 않습니다."));
		return FVector::ZeroVector;
	}

	// 1. 적들의 평균 위치(Centroid) 계산
	FVector EnemyCentroid = FVector::ZeroVector;
	int32 ValidCount = 0;
	for (const TWeakObjectPtr<AActor>& WeakTarget : CachedTargets)
	{
		if (AActor* Target = WeakTarget.Get())
		{
			EnemyCentroid += Target->GetActorLocation();
			++ValidCount;
		}
	}

	if (ValidCount == 0)
	{
		return FVector::ZeroVector;
	}

	EnemyCentroid /= ValidCount;

	// 2. 적 중심에서 반대 방향 벡터 (XY 평면)
	const FVector AIPos = SelfRef->GetActorLocation();
	FVector RetreatDir = AIPos - EnemyCentroid;
	RetreatDir.Z = 0.0f; // 수평 이동만 고려

	if (RetreatDir.IsNearlyZero())
	{
		// AI가 적 중심에 겹쳐있으면 임의 방향 (전방)
		RetreatDir = SelfRef->GetActorForwardVector();
	}

	RetreatDir.Normalize();

	// 3. 잔여 이동력 범위 내 최대 거리 지점
	const FVector CandidatePos = AIPos + RetreatDir * RemainingMP;

	// 4. NavMesh 프로젝션으로 도달 가능 위치 보정
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(
		SelfRef->GetWorld());

	if (!NavSys)
	{
		UE_LOG(LogPBUtility, Warning,
		       TEXT("[Fallback] NavigationSystem을 찾을 수 없습니다."));
		return FVector::ZeroVector;
	}

	FNavLocation ProjectedLocation;
	const bool bProjected = NavSys->ProjectPointToNavigation(
		CandidatePos, ProjectedLocation, FVector(200.0f, 200.0f, 200.0f));

	if (!bProjected)
	{
		// NavMesh에 투영 실패 → 거리를 절반으로 줄여 재시도
		const FVector HalfCandidate = AIPos + RetreatDir * (RemainingMP * 0.5f);
		const bool bHalfProjected = NavSys->ProjectPointToNavigation(
			HalfCandidate, ProjectedLocation, FVector(200.0f, 200.0f, 200.0f));

		if (!bHalfProjected)
		{
			UE_LOG(LogPBUtility, Warning,
			       TEXT("[Fallback] NavMesh 투영 실패. 후퇴 불가."));
			return FVector::ZeroVector;
		}
	}

	UE_LOG(LogPBUtility, Display,
	       TEXT("[Fallback] 후퇴 위치 계산 완료: (%s) → (%s), 적 Centroid: (%s)"),
	       *AIPos.ToCompactString(),
	       *ProjectedLocation.Location.ToCompactString(),
	       *EnemyCentroid.ToCompactString());

	return ProjectedLocation.Location;
}
