#include "PBUtilityClearinghouse.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "PBAIArchetypeData.h"
#include "PBAIMockCharacter.h"
#include "PBGE_RestoreTurnResources.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility_Targeted.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemLibrary.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"
#include "ProjectB3/PBGameplayTags.h"
#include "VisualLogger/VisualLogger.h"

// 임시 로깅을 위한 로그 카테고리 정의
DEFINE_LOG_CATEGORY_STATIC(LogPBUtility, Log, All);

/*~ 정규화 데이터 제공 인터페이스 ~*/

float UPBUtilityClearinghouse::GetNormalizedDistanceToTarget(
	AActor *TargetActor) const
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
	if (const float *CachedValue = CachedDistanceMap.Find(TargetActor))
	{
		return *CachedValue;
	}

	// 경로 비용 기반 정규화 (0.0 ~ 1.0)
	// 거리가 가까울수록 1.0, 최대 이동력 범위를 초과하면 0.0
	float NormalizedScore = 0.0f;

	// CacheTurnData 시점에 캐싱된 최대 이동력 사용
	const float MaxRange = FMath::Max(CachedMaxMovement, 100.0f);

	// EnvironmentSubsystem 경로 비용 조회 (팀 공용 API)
	float PathLength = FVector::Dist(
		ActiveTurnActor->GetActorLocation(),
		TargetActor->GetActorLocation()); // 유클리드 폴백

	if (CachedEnvSubsystem)
	{
		const FPBPathCostResult PathResult = CachedEnvSubsystem->CalculatePathCost(
			ActiveTurnActor->GetActorLocation(),
			TargetActor->GetActorLocation());

		if (PathResult.bIsValid)
		{
			PathLength = PathResult.TotalCost;
		}
	}

	NormalizedScore = FMath::Clamp(1.0f - (PathLength / MaxRange), 0.0f, 1.0f);

	// 캐싱 (턴 내 중복 NavMesh 조회 방지)
	const_cast<UPBUtilityClearinghouse*>(this)->CachedDistanceMap.Add(
		TargetActor, NormalizedScore);

	UE_LOG(
		LogPBUtility, Log,
		TEXT("AI [%s]가 타겟 [%s]과의 '거리 정규화 점수'를 요청함 "
			 "-> PathLength=%.0f, MaxRange=%.0f, Score=%f"),
		*ActiveTurnActor->GetName(), *TargetActor->GetName(),
		PathLength, MaxRange, NormalizedScore);

	return NormalizedScore;
}

float UPBUtilityClearinghouse::GetTargetVulnerabilityScore(
	AActor *TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return 0.0f;
	}

	// 캐싱 데이터 우선 반환
	if (const float *CachedValue = CachedVulnerabilityMap.Find(TargetActor))
	{
		return *CachedValue;
	}

	// HP/MaxHP 기반 취약성 점수 산출
	// HP가 낮을수록 취약(= 마무리 우선) → 1.0 - (HP / MaxHP)
	float CalculatedVulnerabilityScore = 0.5f; // ASC 미발견 시 중립 기본값

	if (const UAbilitySystemComponent *TargetASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
	{
		bool bFound = false;
		const float HP = TargetASC->GetGameplayAttributeValue(
			UPBCharacterAttributeSet::GetHPAttribute(), bFound);

		float MaxHP = 1.0f;
		if (bFound)
		{
			bool bMaxFound = false;
			MaxHP = TargetASC->GetGameplayAttributeValue(
				UPBCharacterAttributeSet::GetMaxHPAttribute(), bMaxFound);

			if (!bMaxFound || MaxHP <= 0.0f)
			{
				MaxHP = 1.0f;
			}

			// HP가 낮을수록 높은 취약성 (마무리 우선순위)
			CalculatedVulnerabilityScore = FMath::Clamp(1.0f - (HP / MaxHP), 0.0f, 1.0f);
		}
	}

	UE_LOG(LogPBUtility, Log,
		   TEXT("AI [%s]가 타겟 [%s]의 '취약성 점수'를 요청함 -> 반환값: %f"),
		   *(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
		   *TargetActor->GetName(), CalculatedVulnerabilityScore);

	return CalculatedVulnerabilityScore;
}

float UPBUtilityClearinghouse::EvaluateHighGroundAdvantage(
	AActor *TargetActor) const
{
	if (!IsValid(TargetActor) || !IsValid(ActiveTurnActor))
	{
		return 0.0f;
	}

	// EnvironmentSubsystem의 LoS 결과에서 ElevationDelta 활용
	// BG3 기준 고지대 = 2.5m(250cm) 이상 높은 위치에서 이점(Advantage) 부여
	constexpr float HeightThreshold = 250.0f; // 팀 공용 기준 (PBLoS_Trace와 동일)

	float HeightDiff = 0.0f;

	if (CachedEnvSubsystem)
	{
		const FPBLoSResult LoSResult = CachedEnvSubsystem->CheckLineOfSight(
			ActiveTurnActor->GetActorLocation(), TargetActor);
		// ElevationDelta: 양수 = AI가 높음 (EnvironmentSubsystem 기준)
		HeightDiff = LoSResult.ElevationDelta;
	}
	else
	{
		// 폴백: 직접 Z축 비교
		HeightDiff = ActiveTurnActor->GetActorLocation().Z
			- TargetActor->GetActorLocation().Z;
	}

	float HighGroundScore = 0.0f;
	if (HeightDiff > 0.0f)
	{
		// AI가 더 높음 → 보너스 (최대 1.0)
		HighGroundScore = FMath::Clamp(HeightDiff / HeightThreshold, 0.0f, 1.0f);
	}
	// HeightDiff <= 0 → AI가 같거나 낮음 → 보너스 없음 (0.0)

	UE_LOG(LogPBUtility, Log,
		   TEXT("AI [%s]가 타겟 [%s]에 대한 '고지대 이점'을 요청함 -> "
				"HeightDiff=%.0fcm, Score=%f"),
		   *ActiveTurnActor->GetName(), *TargetActor->GetName(),
		   HeightDiff, HighGroundScore);

	return HighGroundScore;
}

/*~ 캐싱 (라이프사이클) 관리 인터페이스 ~*/

void UPBUtilityClearinghouse::RestoreTurnResources(AActor *CurrentTurnActor)
{
	if (!IsValid(CurrentTurnActor))
	{
		return;
	}

	APBAIMockCharacter *MockChar = Cast<APBAIMockCharacter>(CurrentTurnActor);
	if (!MockChar)
		return;

	UAbilitySystemComponent *ASC = MockChar->GetAbilitySystemComponent();
	const UPBTurnResourceAttributeSet *AttrSet = MockChar->GetTurnResourceAttributeSet();

	if (ASC && AttrSet)
	{
		// GE_RestoreTurnResources 에셋을 생성하여 턴 시작 자원을 최대치로 초기화
		UGameplayEffect *RestoreGE = NewObject<UPBGE_RestoreTurnResources>(
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

void UPBUtilityClearinghouse::CacheTurnData(AActor *CurrentTurnActor)
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

	// EnvironmentSubsystem 획득 (GameInstanceSubsystem)
	if (UGameInstance* GI = CurrentTurnActor->GetGameInstance())
	{
		CachedEnvSubsystem = GI->GetSubsystem<UPBEnvironmentSubsystem>();
	}

	// LoS 캐시 세션 시작 — 턴 평가 중 Trace 결과 재사용
	if (CachedEnvSubsystem)
	{
		CachedEnvSubsystem->BeginLoSCache();
	}

	UE_LOG(LogPBUtility, Display,
		   TEXT("=== AI [%s]의 턴 시작. 클리어링하우스 캐싱 작업 개시 ==="),
		   *ActiveTurnActor->GetName());

	// 반경 N미터 내의 적대적 액터들을 긁어모아(Overlap 등), 거리 및 취약성
	// 점수를 미리 계산하여 TMap에 적재하는 로직 구현.
	// 현 단계에서는 "Player" 태그를 가진 모든 액터를 타겟
	TArray<AActor *> FoundPlayers;
	UGameplayStatics::GetAllActorsWithTag(CurrentTurnActor->GetWorld(),
										  FName("Player"), FoundPlayers);

	for (AActor *PlayerActor : FoundPlayers)
	{
		if (!IsValid(PlayerActor))
		{
			continue;
		}

		// 사망 캐릭터 제외: Character_State_Dead 태그 보유 시 타겟 목록에서 배제
		UAbilitySystemComponent *TargetASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerActor);
		if (TargetASC && TargetASC->HasMatchingGameplayTag(
							 PBGameplayTags::Character_State_Dead))
		{
			UE_LOG(LogPBUtility, Log,
				   TEXT("사망 캐릭터 [%s]를 타겟 목록에서 제외합니다."),
				   *PlayerActor->GetName());
			continue;
		}

		CachedTargets.Add(PlayerActor);
	}

	UE_LOG(LogPBUtility, Log,
		   TEXT("주변 타겟 탐색 완료. 총 %d명의 잠재적 타겟 목록 캐싱 완료."),
		   CachedTargets.Num());

	// --- ThreatScore 사전 계산 및 정규화 ---
	// AI Scoring Example.md §5: ThreatMultiplier = lerp(0.5, 2.0, NormalizedThreat)
	TMap<AActor*, float> RawThreatScores;
	float MaxThreatScore = 0.0f;

	for (const TWeakObjectPtr<AActor>& WeakTarget : CachedTargets)
	{
		if (!WeakTarget.IsValid())
		{
			continue;
		}

		AActor* Target = WeakTarget.Get();
		const EPBCombatRole Role = DetermineCombatRole(Target);

		// 역할별 기본 위협도
		float RoleBaseThreat;
		switch (Role)
		{
		case EPBCombatRole::Healer: RoleBaseThreat = RoleThreat_Healer; break;
		case EPBCombatRole::Caster: RoleBaseThreat = RoleThreat_Caster; break;
		case EPBCombatRole::Ranged: RoleBaseThreat = RoleThreat_Ranged; break;
		case EPBCombatRole::Tank:   RoleBaseThreat = RoleThreat_Tank;   break;
		default:                    RoleBaseThreat = RoleThreat_Melee;  break;
		}

		// LowHP 축: 빈사 상태일수록 위협 증가 (처리 우선)
		const float VulnerabilityScore = GetTargetVulnerabilityScore(Target);
		const float ThreatScore = RoleBaseThreat + VulnerabilityScore * LowHPThreatWeight;

		RawThreatScores.Add(Target, ThreatScore);
		MaxThreatScore = FMath::Max(MaxThreatScore, ThreatScore);
	}

	// 정규화 → ThreatMultiplier 캐싱
	for (const auto& Pair : RawThreatScores)
	{
		const float NormalizedThreat = (MaxThreatScore > 0.0f)
			? Pair.Value / MaxThreatScore
			: 0.5f;
		const float ThreatMultiplier = FMath::Lerp(0.5f, 2.0f, NormalizedThreat);
		CachedThreatMultiplierMap.Add(Pair.Key, ThreatMultiplier);

		UE_LOG(LogPBUtility, Log,
			TEXT("[ThreatCache] 타겟 [%s]: Role=%d, RawThreat=%.2f, "
				 "Normalized=%.2f, Multiplier=%.2f"),
			*Pair.Key->GetName(),
			static_cast<int32>(DetermineCombatRole(Pair.Key)),
			Pair.Value, NormalizedThreat, ThreatMultiplier);
	}

	// --- ArchetypeWeight 사전 캐싱 (카테고리별 1회 Cast) ---
	CachedArchetypeWeights = FPBCachedArchetypeWeights();
	if (const APBAIMockCharacter* AIChar = Cast<APBAIMockCharacter>(CurrentTurnActor))
	{
		if (const UPBAIArchetypeData* Archetype = AIChar->ArchetypeData)
		{
			CachedArchetypeWeights.AttackWeight  = Archetype->AttackWeight;
			CachedArchetypeWeights.HealWeight    = Archetype->HealWeight;
			CachedArchetypeWeights.BuffWeight    = Archetype->BuffWeight;
			CachedArchetypeWeights.DebuffWeight  = Archetype->DebuffWeight;
			CachedArchetypeWeights.ControlWeight = Archetype->ControlWeight;

			UE_LOG(LogPBUtility, Log,
				TEXT("[Archetype] %s 캐싱 완료: Atk=%.2f, Heal=%.2f, "
					 "Buff=%.2f, Debuff=%.2f, Ctrl=%.2f"),
				*CurrentTurnActor->GetName(),
				CachedArchetypeWeights.AttackWeight,
				CachedArchetypeWeights.HealWeight,
				CachedArchetypeWeights.BuffWeight,
				CachedArchetypeWeights.DebuffWeight,
				CachedArchetypeWeights.ControlWeight);
		}
		else
		{
			UE_LOG(LogPBUtility, Log,
				TEXT("[Archetype] %s에 ArchetypeData 미설정 → 기본 가중치 1.0 사용"),
				*CurrentTurnActor->GetName());
		}
	}

	// --- MaxMovement 캐싱 ---
	// TurnResourceAttributeSet의 명시적 MaxMovement 속성 사용
	CachedMaxMovement = 1000.0f; // 폴백 기본값
	if (UAbilitySystemComponent* TurnASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CurrentTurnActor))
	{
		if (const UPBTurnResourceAttributeSet* TurnRes =
				TurnASC->GetSet<UPBTurnResourceAttributeSet>())
		{
			CachedMaxMovement = TurnRes->GetMaxMovement();
		}
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("[MovementCache] %s MaxMovement=%.0f cm 캐싱 완료"),
		*CurrentTurnActor->GetName(), CachedMaxMovement);

	// --- 아군 캐싱 ---
	// "Enemy" 태그 액터 중 사망자 제외, Self 포함
	TArray<AActor*> FoundAllies;
	UGameplayStatics::GetAllActorsWithTag(
		CurrentTurnActor->GetWorld(), FName("Enemy"), FoundAllies);

	// Self를 명시적으로 아군 목록에 추가 (자가 힐/버프 후보)
	CachedAllies.Add(CurrentTurnActor);

	for (AActor* AllyActor : FoundAllies)
	{
		if (!IsValid(AllyActor) || AllyActor == CurrentTurnActor)
		{
			continue;
		}

		UAbilitySystemComponent* AllyASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AllyActor);
		if (AllyASC && AllyASC->HasMatchingGameplayTag(
						   PBGameplayTags::Character_State_Dead))
		{
			UE_LOG(LogPBUtility, Log,
				TEXT("사망 아군 [%s]를 아군 목록에서 제외합니다."),
				*AllyActor->GetName());
			continue;
		}

		CachedAllies.Add(AllyActor);
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("아군 탐색 완료. 총 %d명 (Self 포함) 아군 목록 캐싱 완료."),
		CachedAllies.Num());

	// Visual Logger: 턴 시작 상태 스냅샷
	UE_VLOG(CurrentTurnActor, LogPBUtility, Log,
		TEXT("[CacheTurnData] Targets=%d, Allies=%d, MaxMP=%.0f, "
			 "Archetype(Atk=%.1f Heal=%.1f Buff=%.1f Debuff=%.1f Ctrl=%.1f)"),
		CachedTargets.Num(), CachedAllies.Num(), CachedMaxMovement,
		CachedArchetypeWeights.AttackWeight, CachedArchetypeWeights.HealWeight,
		CachedArchetypeWeights.BuffWeight, CachedArchetypeWeights.DebuffWeight,
		CachedArchetypeWeights.ControlWeight);
}

void UPBUtilityClearinghouse::ClearCache()
{
	// 맵 컨테이너 요소들을 모두 비워 다음 턴이나 다른 캐릭터 연산 시 간섭이
	// 없도록 한다.
	CachedTargets.Empty();
	CachedAllies.Empty();
	CachedDistanceMap.Empty();
	CachedVulnerabilityMap.Empty();
	CachedThreatMultiplierMap.Empty();
	CachedActionScoreMap.Empty();
	CachedAPAttackScoreMap.Empty();
	CachedBAAttackScoreMap.Empty();
	CachedHealScoreMap.Empty();
	CachedArchetypeWeights = FPBCachedArchetypeWeights();
	CachedMaxMovement = 1000.0f;
	EQSTargetActor = nullptr;

	// LoS 캐시 세션 종료 — Trace 결과 캐시 파기
	if (CachedEnvSubsystem)
	{
		CachedEnvSubsystem->EndLoSCache();
	}

	UE_LOG(LogPBUtility, Log,
		   TEXT("클리어링하우스 메모리 캐시가 성공적으로 비워졌습니다."));
}

/*~ 헬퍼 함수 ~*/

EPBCombatRole UPBUtilityClearinghouse::DetermineCombatRole(AActor *TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return EPBCombatRole::Melee;
	}

	UAbilitySystemComponent *ASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!IsValid(ASC))
	{
		return EPBCombatRole::Melee;
	}

	// Character.Class.* 태그 기반 역할 매핑
	if (ASC->HasMatchingGameplayTag(PBGameplayTags::Character_Class_Magician))
	{
		return EPBCombatRole::Caster;
	}
	if (ASC->HasMatchingGameplayTag(PBGameplayTags::Character_Class_Ranger))
	{
		return EPBCombatRole::Ranged;
	}
	// Fighter 또는 태그 없음 → Melee
	return EPBCombatRole::Melee;
}

float UPBUtilityClearinghouse::GetRoleMultiplier(EPBCombatRole TargetRole)
{
	// AI Scoring Example.md §5: Attack 행동 × 타겟 역할 가중치
	// | Attack | Healer:1.3 | Caster:1.2 | Ranged:1.1 | Melee:1.0 | Tank:0.8 |
	switch (TargetRole)
	{
	case EPBCombatRole::Healer: return 1.3f;
	case EPBCombatRole::Caster: return 1.2f;
	case EPBCombatRole::Ranged: return 1.1f;
	case EPBCombatRole::Tank:   return 0.8f;
	default:                    return 1.0f;
	}
}

FPBCostData UPBUtilityClearinghouse::ExtractAbilityCost(
	const UAbilitySystemComponent* ASC,
	const FGameplayAbilitySpecHandle& SpecHandle)
{
	// 폴백: Cost GE가 없거나 턴 자원 Modifier가 없으면 AP 1 소모
	FPBCostData Fallback;
	Fallback.ActionCost = 1.0f;

	if (!IsValid(ASC) || !SpecHandle.IsValid())
	{
		return Fallback;
	}

	const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(SpecHandle);
	if (!Spec || !IsValid(Spec->Ability))
	{
		return Fallback;
	}

	// 어빌리티의 Cost GE CDO 획득 — 없으면 태그 기반 폴백으로 이동
	const UGameplayEffect* CostGE = Spec->Ability->GetCostGameplayEffect();

	// Cost GE의 Modifier를 순회하며 턴 자원(AP/BA/MP) 매핑
	bool bFoundTurnResource = false;
	FPBCostData ExtractedCost;

	if (!CostGE)
	{
		// Cost GE 없음 → 아래 태그 폴백으로 이동
	}
	else for (const FGameplayModifierInfo& Mod : CostGE->Modifiers)
	{
		float Magnitude = 0.0f;
		// ScalableFloat 기반 정적 값 추출 (Level 1 기준)
		if (!Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.0f, Magnitude))
		{
			continue;
		}

		// Cost GE는 차감용 음수 → 절대값으로 변환
		const float PositiveCost = FMath::Abs(Magnitude);

		if (Mod.Attribute == UPBTurnResourceAttributeSet::GetActionAttribute())
		{
			ExtractedCost.ActionCost = PositiveCost;
			bFoundTurnResource = true;
		}
		else if (Mod.Attribute == UPBTurnResourceAttributeSet::GetBonusActionAttribute())
		{
			ExtractedCost.BonusActionCost = PositiveCost;
			bFoundTurnResource = true;
		}
		else if (Mod.Attribute == UPBTurnResourceAttributeSet::GetMovementAttribute())
		{
			ExtractedCost.MovementCost = PositiveCost;
			bFoundTurnResource = true;
		}
	}

	if (bFoundTurnResource)
	{
		UE_LOG(LogPBUtility, Verbose,
			TEXT("[ExtractAbilityCost] %s → AP=%.1f, BA=%.1f, MP=%.1f"),
			*GetNameSafe(Spec->Ability),
			ExtractedCost.ActionCost, ExtractedCost.BonusActionCost,
			ExtractedCost.MovementCost);
		return ExtractedCost;
	}

	// 턴 자원 Modifier 없음 → DynamicAbilityTags 기반 폴백
	// Ability.Type.BonusAction 태그가 있으면 BA 1 소모, 그 외 AP 1 소모
	const EPBAbilityType AbilityType = GetAbilityTypeFromTags(Spec->GetDynamicSpecSourceTags());
	FPBCostData TagFallback;
	switch (AbilityType)
	{
	case EPBAbilityType::BonusAction:
		TagFallback.BonusActionCost = 1.0f;
		break;
	case EPBAbilityType::Free:
		// Free: 자원 소모 없음
		break;
	default:
		// Action, Reaction, None → AP 1 소모
		TagFallback.ActionCost = 1.0f;
		break;
	}

	UE_LOG(LogPBUtility, Verbose,
		TEXT("[ExtractAbilityCost] %s: Cost GE 턴자원 없음 → 태그 폴백 (Type=%d, AP=%.1f, BA=%.1f)"),
		*GetNameSafe(Spec->Ability), static_cast<int32>(AbilityType),
		TagFallback.ActionCost, TagFallback.BonusActionCost);
	return TagFallback;
}

/*~ 스코어링 (ActionScore 산출) ~*/

FPBTargetScore
UPBUtilityClearinghouse::EvaluateActionScore(AActor *TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return FPBTargetScore{};
	}

	// 캐시 먼저 확인 (턴 내 중복 연산 방지)
	if (const FPBTargetScore *Cached = CachedActionScoreMap.Find(TargetActor))
	{
		return *Cached;
	}

	FPBTargetScore Score;
	Score.TargetActor = TargetActor;

	// --- ExpectedDamage 산정 ---
	// SourceASC의 모든 활성 어빌리티를 순회하여 최고 기대 피해량 + 해당 AbilityTag 추출
	// CDO에서 DiceSpec을 읽고, PBAbilitySystemLibrary 정적 함수로 안전하게 계산
	UAbilitySystemComponent *SourceASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActiveTurnActor);
	UAbilitySystemComponent *TargetASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

	float BestExpectedDamage = 0.0f;
	FGameplayTag BestAbilityTag;
	FGameplayAbilitySpecHandle BestSpecHandle;
	bool bBestCanKill = false;

	// AP/BA 자원 유형별 최고 어빌리티 추적 (DFS 후보 분리용)
	float BestAPDamage = 0.0f;
	FGameplayTag BestAPTag;
	FGameplayAbilitySpecHandle BestAPHandle;
	bool bBestAPCanKill = false;

	float BestBADamage = 0.0f;
	FGameplayTag BestBATag;
	FGameplayAbilitySpecHandle BestBAHandle;
	bool bBestBACanKill = false;

	if (IsValid(SourceASC) && IsValid(TargetASC))
	{
		// 태그 컨텍스트 (향후 저항/취약 등 확장용)
		FGameplayTagContainer SourceTags;
		SourceASC->GetOwnedGameplayTags(SourceTags);
		FGameplayTagContainer TargetTags;
		TargetASC->GetOwnedGameplayTags(TargetTags);

		// 타겟 AC (HitRoll 경로용)
		bool bACFound = false;
		const int32 TargetAC = static_cast<int32>(TargetASC->GetGameplayAttributeValue(
			UPBCharacterAttributeSet::GetArmorClassAttribute(), bACFound));

		// 타겟 현재 HP (KillBonus / OverhealPenalty 판정용)
		bool bHPFound = false;
		const float TargetCurrentHP = TargetASC->GetGameplayAttributeValue(
			UPBCharacterAttributeSet::GetHPAttribute(), bHPFound);

		const TArray<FGameplayAbilitySpec> &Specs = SourceASC->GetActivatableAbilities();
		UE_LOG(LogTemp, Warning,TEXT("found Specs Num : %d") , Specs.Num());
		for (const FGameplayAbilitySpec &Spec : Specs)
		{
			
			// 발동 불가 어빌리티 스킵 (쿨다운, 자원 부족 등)
			// UGameplayAbility::CanActivateAbility는 public이므로 Spec.Ability를 통해 호출
			if (!Spec.Ability || !Spec.Ability->CanActivateAbility(
									 Spec.Handle, SourceASC->AbilityActorInfo.Get()))
			{
				continue;
			}

			const UPBGameplayAbility *AbilityCDO = Cast<UPBGameplayAbility>(Spec.Ability);
			if (!AbilityCDO)
			{
				continue;
			}

			// Targeted 어빌리티만 평가 (AI Execute 파이프라인이 Payload 기반이므로)
			if (!Cast<UPBGameplayAbility_Targeted>(Spec.Ability))
			{
				continue;
			}

			// Attack 카테고리만 평가 (Heal/Buff/Control 등 제외)
			if (AbilityCDO->GetAbilityCategory() != EPBAbilityCategory::Attack)
			{
				continue;
			}

			const FPBDiceSpec &Dice = AbilityCDO->GetDiceSpec();

			// 데미지 주사위가 없는 어빌리티는 스킵 (이동, 버프 등)
			if (Dice.DiceCount <= 0 || Dice.DiceFaces <= 0)
			{
				continue;
			}

			float CandidateDamage = 0.0f;
			int32 AtkMod = 0;

			switch (Dice.RollType)
			{
			case EPBDiceRollType::HitRoll:
			{
				const int32 HitBonus = UPBAbilitySystemLibrary::GetHitBonus(
					SourceASC, Dice.BonusAttributeOverride);
				AtkMod = UPBAbilitySystemLibrary::GetAttackModifier(
					SourceASC, Dice.AttackModifierAttributeOverride);

				CandidateDamage = UPBAbilitySystemLibrary::CalcExpectedAttackDamage(
					Dice.DiceCount, Dice.DiceFaces, static_cast<float>(AtkMod),
					HitBonus, TargetAC, SourceTags, TargetTags);
				break;
			}

			case EPBDiceRollType::SavingThrow:
			{
				const int32 SpellDC = UPBAbilitySystemLibrary::CalcSpellSaveDC(
					SourceASC, Dice.BonusAttributeOverride);
				const int32 SaveBonus = UPBAbilitySystemLibrary::GetSaveBonus(
					TargetASC, Dice.TargetSaveAttribute);
				AtkMod = UPBAbilitySystemLibrary::GetAttackModifier(
					SourceASC, Dice.AttackModifierAttributeOverride);

				CandidateDamage = UPBAbilitySystemLibrary::CalcExpectedSavingThrowDamage(
					Dice.DiceCount, Dice.DiceFaces, static_cast<float>(AtkMod),
					SaveBonus, SpellDC, SourceTags, TargetTags);
				break;
			}

			case EPBDiceRollType::None:
			{
				AtkMod = UPBAbilitySystemLibrary::GetAttackModifier(
					SourceASC, Dice.AttackModifierAttributeOverride);

				CandidateDamage = UPBAbilitySystemLibrary::CalcExpectedDamage(
					Dice.DiceCount, Dice.DiceFaces, static_cast<float>(AtkMod),
					SourceTags, TargetTags);
				break;
			}
			}

			// --- KillBonus / OverhealPenalty 적용 (AI Scoring Example.md §3.1) ---
			// RawAvgDamage = 명중 시 평균 피해 (확률 미반영)
			// bCanKill = 명중하면 처치 가능한가?
			bool bCandidateCanKill = false;
			if (bHPFound && TargetCurrentHP > 0.0f)
			{
				const float RawAvgDamage =
					Dice.DiceCount * (Dice.DiceFaces + 1) / 2.0f
					+ static_cast<float>(AtkMod);

				bCandidateCanKill = (RawAvgDamage >= TargetCurrentHP);

				if (bCandidateCanKill)
				{
					// OverhealPenalty: 초과 데미지 비율만큼 기대값 축소
					// KillBonus: 처치 보상으로 상쇄 (1.0 + KillBonusRate)
					const float EffectiveRatio =
						TargetCurrentHP / FMath::Max(RawAvgDamage, 1.0f);
					CandidateDamage *= EffectiveRatio * (1.0f + KillBonusRate);
				}
				// else: 초과 피해 없으므로 수정 없음
			}

			// --- 자원 유형별 최고 어빌리티 추적 ---
			const FPBCostData AbilityCost = ExtractAbilityCost(SourceASC, Spec.Handle);
			const bool bIsBonusAction = (AbilityCost.BonusActionCost > 0.0f);

			if (bIsBonusAction)
			{
				if (CandidateDamage > BestBADamage)
				{
					BestBADamage = CandidateDamage;
					BestBAHandle = Spec.Handle;
					bBestBACanKill = bCandidateCanKill;
					const FGameplayTagContainer& Tags = AbilityCDO->GetAssetTags();
					BestBATag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();
				}
			}
			else
			{
				if (CandidateDamage > BestAPDamage)
				{
					BestAPDamage = CandidateDamage;
					BestAPHandle = Spec.Handle;
					bBestAPCanKill = bCandidateCanKill;
					const FGameplayTagContainer& Tags = AbilityCDO->GetAssetTags();
					BestAPTag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();
				}
			}

			if (CandidateDamage > BestExpectedDamage)
			{
				BestExpectedDamage = CandidateDamage;
				BestSpecHandle = Spec.Handle;
				bBestCanKill = bCandidateCanKill;

				// AbilityTag는 로깅/디버그용으로 유지 (AssetTags에서 추출)
				const FGameplayTagContainer &AbilityTags = AbilityCDO->GetAssetTags();
				BestAbilityTag = AbilityTags.Num() > 0
					? AbilityTags.First()
					: FGameplayTag();
			}
		}
	}

	Score.ExpectedDamage = BestExpectedDamage;
	Score.AbilityTag = BestAbilityTag;
	Score.AbilitySpecHandle = BestSpecHandle;

	// --- TargetModifier 산정 ---
	// AI Scoring Example.md §5: TargetModifier = ThreatMultiplier × RoleMultiplier
	const float* CachedThreat = CachedThreatMultiplierMap.Find(TargetActor);
	const float ThreatMultiplier = CachedThreat ? *CachedThreat : 1.0f;
	const EPBCombatRole TargetRole = DetermineCombatRole(TargetActor);
	const float RoleMultiplier = GetRoleMultiplier(TargetRole);
	Score.TargetModifier = ThreatMultiplier * RoleMultiplier;

	// --- SituationalBonus 산정 ---
	Score.SituationalBonus = 0.0f;

	// FinishOffBonus (AI Scoring Example.md §6):
	// 처치 가능 시, 적이 살아있으면 가할 미래 위협을 제거하는 가치
	// = TargetThreatPerTurn × 잔여 라운드 (최대 3)
	if (bBestCanKill)
	{
		Score.SituationalBonus += FinishOffBaseThreat * MaxFinishOffRounds;
		UE_LOG(LogPBUtility, Log,
			TEXT("[KillBonus] 타겟 [%s] 처치 가능 → FinishOffBonus +%.1f 가산"),
			*TargetActor->GetName(),
			FinishOffBaseThreat * MaxFinishOffRounds);
	}
	// TODO: ConcentrationBreakBonus, CliffShoveBonus 등 환경 상호작용

	// --- ArchetypeWeight 산정 ---
	// CacheTurnData에서 사전 캐싱한 카테고리별 가중치 사용 (Cast 없음)
	Score.ArchetypeWeight = CachedArchetypeWeights.AttackWeight;

	// --- MovementScore 산정 ---
	// 공식: 1.0 - (DistToTarget / MaxMovementRange), 클램프 [0.0, 1.0]
	// 거리가 가까울수록 1.0, 멀수록 0.0
	// MaxMovementRange = TurnResourceAttributeSet의 MaxMovement 실값
	// CachedMaxMovement: CacheTurnData 시점(턴 시작)의 Movement 값
	// EvaluateActionScore 호출 시점에는 이미 자원이 소모되었을 수 있으므로
	// 턴 시작 시 캐싱된 최대 이동력을 사용한다.
	const float MaxMovementRange = FMath::Max(CachedMaxMovement, 100.0f);

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
				"ExpDmg=%.2f (Ability=%s), TargetMod=%.2f, "
				"Situational=%.1f, Archetype=%.2f, Move=%.2f → "
				"TotalScore=%.4f"),
		   *(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
		   *TargetActor->GetName(), Score.ExpectedDamage,
		   *Score.AbilityTag.ToString(),
		   Score.TargetModifier, Score.SituationalBonus,
		   Score.ArchetypeWeight, Score.MovementScore,
		   FinalScore);

	// Visual Logger: 타겟 위치에 스코어 마커 표시
	UE_VLOG_LOCATION(ActiveTurnActor, LogPBUtility, Log,
		TargetActor->GetActorLocation(), 30.0f, FColor::Red,
		TEXT("%s: %.1f"), *TargetActor->GetName(), FinalScore);

	// 결과 캐싱 (전체 최고 — 타겟 랭킹용)
	CachedActionScoreMap.Add(TargetActor, Score);

	// --- 자원 유형별 캐시 맵 갱신 (DFS 후보 생성용) ---
	// TargetModifier, ArchetypeWeight, MovementScore는 타겟 수준 속성이므로 공유
	if (BestAPHandle.IsValid())
	{
		FPBTargetScore APScore;
		APScore.TargetActor = TargetActor;
		APScore.ExpectedDamage = BestAPDamage;
		APScore.AbilityTag = BestAPTag;
		APScore.AbilitySpecHandle = BestAPHandle;
		APScore.TargetModifier = Score.TargetModifier;
		APScore.ArchetypeWeight = Score.ArchetypeWeight;
		APScore.MovementScore = Score.MovementScore;
		APScore.SituationalBonus = bBestAPCanKill
			? FinishOffBaseThreat * MaxFinishOffRounds : 0.0f;
		CachedAPAttackScoreMap.Add(TargetActor, APScore);
	}
	if (BestBAHandle.IsValid())
	{
		FPBTargetScore BAScore;
		BAScore.TargetActor = TargetActor;
		BAScore.ExpectedDamage = BestBADamage;
		BAScore.AbilityTag = BestBATag;
		BAScore.AbilitySpecHandle = BestBAHandle;
		BAScore.TargetModifier = Score.TargetModifier;
		BAScore.ArchetypeWeight = Score.ArchetypeWeight;
		BAScore.MovementScore = Score.MovementScore;
		BAScore.SituationalBonus = bBestBACanKill
			? FinishOffBaseThreat * MaxFinishOffRounds : 0.0f;
		CachedBAAttackScoreMap.Add(TargetActor, BAScore);
	}

	return Score;
}

/*~ Heal 스코어링 ~*/

FPBTargetScore UPBUtilityClearinghouse::EvaluateHealScore(AActor* AllyTarget)
{
	if (!IsValid(AllyTarget))
	{
		return FPBTargetScore{};
	}

	// 캐시 먼저 확인 (턴 내 중복 연산 방지)
	if (const FPBTargetScore* Cached = CachedHealScoreMap.Find(AllyTarget))
	{
		return *Cached;
	}

	FPBTargetScore Score;
	Score.TargetActor = AllyTarget;

	// --- 아군 HP 정보 조회 ---
	UAbilitySystemComponent* AllyASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AllyTarget);
	if (!IsValid(AllyASC))
	{
		CachedHealScoreMap.Add(AllyTarget, Score);
		return Score;
	}

	bool bHPFound = false;
	const float CurrentHP = AllyASC->GetGameplayAttributeValue(
		UPBCharacterAttributeSet::GetHPAttribute(), bHPFound);
	bool bMaxHPFound = false;
	const float MaxHP = AllyASC->GetGameplayAttributeValue(
		UPBCharacterAttributeSet::GetMaxHPAttribute(), bMaxHPFound);

	if (!bHPFound || !bMaxHPFound || MaxHP <= 0.0f)
	{
		CachedHealScoreMap.Add(AllyTarget, Score);
		return Score;
	}

	// 이미 만피 → Heal 가치 없음
	const float MissingHP = MaxHP - CurrentHP;
	if (MissingHP <= 0.0f)
	{
		CachedHealScoreMap.Add(AllyTarget, Score);
		return Score;
	}

	// --- ExpectedHeal 산정 ---
	// SourceASC의 Heal 카테고리 어빌리티 중 최고 기대 회복량 + 해당 AbilityTag 추출
	UAbilitySystemComponent* SourceASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActiveTurnActor);
	if (!IsValid(SourceASC))
	{
		CachedHealScoreMap.Add(AllyTarget, Score);
		return Score;
	}

	float BestExpectedHeal = 0.0f;
	FGameplayTag BestHealTag;
	FGameplayAbilitySpecHandle BestHealSpecHandle;

	const TArray<FGameplayAbilitySpec>& Specs = SourceASC->GetActivatableAbilities();
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		if (!Spec.Ability || !Spec.Ability->CanActivateAbility(
								 Spec.Handle, SourceASC->AbilityActorInfo.Get()))
		{
			continue;
		}

		const UPBGameplayAbility* AbilityCDO = Cast<UPBGameplayAbility>(Spec.Ability);
		if (!AbilityCDO)
		{
			continue;
		}

		// Targeted 어빌리티만 평가 (AI Execute 파이프라인이 Payload 기반이므로)
		if (!Cast<UPBGameplayAbility_Targeted>(Spec.Ability))
		{
			continue;
		}

		// Heal 카테고리만 평가
		if (AbilityCDO->GetAbilityCategory() != EPBAbilityCategory::Heal)
		{
			continue;
		}

		const FPBDiceSpec& Dice = AbilityCDO->GetDiceSpec();
		if (Dice.DiceCount <= 0 || Dice.DiceFaces <= 0)
		{
			continue;
		}

		// 기대 회복량 = DiceCount × (DiceFaces + 1) / 2 + Modifier
		const int32 HealMod = UPBAbilitySystemLibrary::GetAttackModifier(
			SourceASC, Dice.AttackModifierAttributeOverride);
		const float RawExpectedHeal =
			Dice.DiceCount * (Dice.DiceFaces + 1) / 2.0f
			+ static_cast<float>(HealMod);

		if (RawExpectedHeal > BestExpectedHeal)
		{
			BestExpectedHeal = RawExpectedHeal;
			BestHealSpecHandle = Spec.Handle;

			// AbilityTag는 로깅/디버그용 (AssetTags에서 추출)
			const FGameplayTagContainer& AbilityTags = AbilityCDO->GetAssetTags();
			BestHealTag = AbilityTags.Num() > 0
				? AbilityTags.First()
				: FGameplayTag();
		}
	}

	// Heal 어빌리티가 없으면 점수 0
	if (BestExpectedHeal <= 0.0f)
	{
		CachedHealScoreMap.Add(AllyTarget, Score);
		return Score;
	}

	// --- EffectiveHeal (과잉 힐 방지) ---
	const float EffectiveHeal = FMath::Min(BestExpectedHeal, MissingHP);

	// --- UrgencyMultiplier (AI Scoring Example.md §3.2) ---
	// HPRatio 구간별 긴급도 가중치
	const float HPRatio = CurrentHP / MaxHP;
	float UrgencyMultiplier;
	if (HPRatio <= 0.25f)
	{
		UrgencyMultiplier = 2.0f; // 위급
	}
	else if (HPRatio <= 0.50f)
	{
		UrgencyMultiplier = 1.5f; // 위험
	}
	else if (HPRatio <= 0.75f)
	{
		UrgencyMultiplier = 1.0f; // 경상
	}
	else
	{
		UrgencyMultiplier = 0.5f; // 경미
	}

	// --- Score 조립 ---
	// HealBase = EffectiveHeal × UrgencyMultiplier
	// ActionScore = HealBase × HealWeight
	// TargetModifier = 1.0 (아군에 Threat/Role 미적용)
	// SituationalBonus = 0.0 (현재 미구현)
	Score.ExpectedDamage = EffectiveHeal; // FPBTargetScore 재활용: "기대 효과량"
	Score.AbilityTag = BestHealTag;
	Score.AbilitySpecHandle = BestHealSpecHandle;
	Score.TargetModifier = UrgencyMultiplier; // Heal은 UrgencyMultiplier를 여기에 저장
	Score.SituationalBonus = 0.0f;
	Score.ArchetypeWeight = CachedArchetypeWeights.HealWeight;

	const float FinalScore = Score.GetActionScore();

	UE_LOG(LogPBUtility, Log,
		TEXT("[HealScoring] AI [%s] → 아군 [%s]: "
			 "HP=%.0f/%.0f (%.0f%%), EffHeal=%.1f, "
			 "Urgency=%.1f, HealWeight=%.2f → Score=%.2f "
			 "(Ability=%s)"),
		*(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
		*AllyTarget->GetName(),
		CurrentHP, MaxHP, HPRatio * 100.0f,
		EffectiveHeal, UrgencyMultiplier,
		CachedArchetypeWeights.HealWeight,
		FinalScore, *BestHealTag.ToString());

	// 결과 캐싱
	CachedHealScoreMap.Add(AllyTarget, Score);
	return Score;
}

AActor *UPBUtilityClearinghouse::GetBestActionScoreTarget()
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

	for (const TWeakObjectPtr<AActor> &WeakTarget : CachedTargets)
	{
		if (!WeakTarget.IsValid())
		{
			continue;
		}
		AllScores.Add(EvaluateActionScore(WeakTarget.Get()));
	}

	AllScores.Sort([](const FPBTargetScore &A, const FPBTargetScore &B)
				   { return A.GetTotalScore() > B.GetTotalScore(); });

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

/*~ DFS 후보 행동 생성 ~*/

TArray<FPBSequenceAction> UPBUtilityClearinghouse::GetCandidateActions(
	const FPBUtilityContext& Context) const
{
	TArray<FPBSequenceAction> Candidates;

	if (!IsValid(ActiveTurnActor))
	{
		return Candidates;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActiveTurnActor);
	if (!IsValid(SourceASC))
	{
		return Candidates;
	}

	// 공격 후보 생성 헬퍼 (AP/BA 어빌리티 맵 공용)
	// 같은 타겟이라도 AP 어빌리티와 BA 어빌리티가 각각 후보로 등록될 수 있다.
	auto GenerateAttackCandidates = [&](const TMap<AActor*, FPBTargetScore>& ScoreMap)
	{
		for (const auto& Pair : ScoreMap)
		{
			AActor* Target = Pair.Key;
			const FPBTargetScore& ScoreData = Pair.Value;

			if (!IsValid(Target))
			{
				continue;
			}

			// 사망 타겟 스킵
			if (const UAbilitySystemComponent* TargetASC =
					UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
			{
				if (TargetASC->HasMatchingGameplayTag(
						PBGameplayTags::Character_State_Dead))
				{
					continue;
				}
			}

			// 어빌리티 사거리 조회 (SpecHandle 기반 — 태그 검색 불필요)
			float AbilityRange = 0.0f;
			if (ScoreData.AbilitySpecHandle.IsValid())
			{
				if (const FGameplayAbilitySpec* FoundSpec =
						SourceASC->FindAbilitySpecFromHandle(ScoreData.AbilitySpecHandle))
				{
					if (const UPBGameplayAbility_Targeted* TargetedAbility =
							Cast<UPBGameplayAbility_Targeted>(FoundSpec->Ability))
					{
						AbilityRange = TargetedAbility->GetRange();
					}
				}
			}

			// IsTargetInRange과 동일한 2D(XY) 수평 거리 사용 (고도 차이 무시)
			const float DistToTargetXY = FVector::DistXY(
				Context.LastActionLocation, Target->GetActorLocation());
			const bool bUnlimitedRange = (AbilityRange <= 0.0f);
			const bool bInRange = bUnlimitedRange || (DistToTargetXY <= AbilityRange);

			// LoS 판정: 현재 위치에서 타겟에 대한 시야 확보 여부
			bool bHasLoS = true;
			if (CachedEnvSubsystem)
			{
				const FPBLoSResult LoSResult = CachedEnvSubsystem->CheckLineOfSight(
					Context.LastActionLocation, Target);
				bHasLoS = LoSResult.bHasLineOfSight;
			}

			// Attack 후보: Cost GE 기반 자원 검사
			const FPBCostData AttackCost = ExtractAbilityCost(SourceASC, ScoreData.AbilitySpecHandle);
			if (Context.RemainingAP >= AttackCost.ActionCost
				&& Context.RemainingBA >= AttackCost.BonusActionCost)
			{
				FPBSequenceAction AttackAction;
				AttackAction.ActionType = EPBActionType::Attack;
				AttackAction.TargetActor = Target;
				AttackAction.AbilityTag = ScoreData.AbilityTag;
				AttackAction.AbilitySpecHandle = ScoreData.AbilitySpecHandle;
				AttackAction.Cost.ActionCost = AttackCost.ActionCost;
				AttackAction.Cost.BonusActionCost = AttackCost.BonusActionCost;
				AttackAction.CachedActionScore = ScoreData.GetActionScore();

				// 사거리 밖이거나 LoS 없으면 이동 필요
				const bool bNeedsMovement = (!bInRange && !bUnlimitedRange) || !bHasLoS;

				if (bNeedsMovement)
				{
					const float NeededMovement = FMath::Max(
						DistToTargetXY - AbilityRange, 1.0f);
					if (Context.CanReachTarget(Target->GetActorLocation()))
					{
						AttackAction.Cost.MovementCost = NeededMovement;
					}
					else
					{
						continue;  // 도달 불가 → 후보 제외
					}
				}

				Candidates.Add(AttackAction);
			}
		}
	};

	// AP/BA 양쪽 어빌리티 맵에서 공격 후보 생성
	GenerateAttackCandidates(CachedAPAttackScoreMap);
	GenerateAttackCandidates(CachedBAAttackScoreMap);

	// --- Heal 후보: CachedHealScoreMap에서 아군 대상 ---
	for (const auto& HealPair : CachedHealScoreMap)
	{
		AActor* Ally = HealPair.Key;
		const FPBTargetScore& HealData = HealPair.Value;

		if (!IsValid(Ally))
		{
			continue;
		}

		// 점수 0 이하 (만피 등) → 후보 불필요
		if (HealData.GetActionScore() <= 0.0f)
		{
			continue;
		}

		// Cost GE 기반 자원 검사
		const FPBCostData HealCost = ExtractAbilityCost(SourceASC, HealData.AbilitySpecHandle);
		if (Context.RemainingAP < HealCost.ActionCost
			|| Context.RemainingBA < HealCost.BonusActionCost)
		{
			continue;
		}

		// 사거리 검사 (SpecHandle 기반)
		float HealRange = 0.0f;
		if (HealData.AbilitySpecHandle.IsValid())
		{
			if (const FGameplayAbilitySpec* FoundSpec =
					SourceASC->FindAbilitySpecFromHandle(HealData.AbilitySpecHandle))
			{
				if (const UPBGameplayAbility_Targeted* TargetedAbility =
						Cast<UPBGameplayAbility_Targeted>(FoundSpec->Ability))
				{
					HealRange = TargetedAbility->GetRange();
				}
			}
		}

		// IsTargetInRange과 동일한 2D(XY) 수평 거리 사용
		const float DistToAllyXY = FVector::DistXY(
			Context.LastActionLocation, Ally->GetActorLocation());
		const bool bHealUnlimited = (HealRange <= 0.0f);
		const bool bHealInRange = bHealUnlimited || (DistToAllyXY <= HealRange);

		// LoS 판정 (Heal도 시야 필요 — 벽 뒤 아군 힐 불가)
		bool bHealHasLoS = true;
		if (CachedEnvSubsystem)
		{
			const FPBLoSResult LoSResult = CachedEnvSubsystem->CheckLineOfSight(
				Context.LastActionLocation, Ally);
			bHealHasLoS = LoSResult.bHasLineOfSight;
		}

		// Phase 3: Heal도 Attack과 동일하게 사거리 밖/LoS 없으면 MovementCost 부착
		{
			FPBSequenceAction HealAction;
			HealAction.ActionType = EPBActionType::Heal;
			HealAction.TargetActor = Ally;
			HealAction.AbilityTag = HealData.AbilityTag;
			HealAction.AbilitySpecHandle = HealData.AbilitySpecHandle;
			HealAction.Cost.ActionCost = HealCost.ActionCost;
			HealAction.Cost.BonusActionCost = HealCost.BonusActionCost;
			HealAction.CachedActionScore = HealData.GetActionScore();

			const bool bHealNeedsMovement =
				(!bHealInRange && !bHealUnlimited) || !bHealHasLoS;

			if (bHealNeedsMovement)
			{
				const float NeededMovement = FMath::Max(
					DistToAllyXY - HealRange, 1.0f);
				if (Context.CanReachTarget(Ally->GetActorLocation()))
				{
					HealAction.Cost.MovementCost = NeededMovement;
				}
				else
				{
					continue;  // 도달 불가
				}
			}

			Candidates.Add(HealAction);
		}
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("[DFS] GetCandidateActions: %d개 후보 행동 생성 "
			 "(AP=%.0f, BA=%.0f, MP잔여=%.0f, 누적MP=%.0f)"),
		Candidates.Num(),
		Context.RemainingAP, Context.RemainingBA,
		Context.RemainingMP, Context.AccumulatedMP);

	return Candidates;
}

/*~ DFS 다중 행동 탐색 ~*/

void UPBUtilityClearinghouse::SearchBestSequence(
	FPBUtilityContext Context,
	TArray<FPBSequenceAction>& CurrentPath,
	float CurrentScore,
	float& BestScore,
	TArray<FPBSequenceAction>& BestPath,
	int32 Depth)
{
	// 현재 경로 평가: 비어있지 않고 기존 최고점을 초과하면 갱신
	// ("아무것도 안 하기"는 BestPath에 포함하지 않음 — Fallback에서 별도 처리)
	if (!CurrentPath.IsEmpty() && CurrentScore > BestScore)
	{
		BestScore = CurrentScore;
		BestPath = CurrentPath;

		// Visual Logger: 최적 경로 갱신 기록
		UE_VLOG(ActiveTurnActor, LogPBUtility, Log,
			TEXT("[DFS] BestPath 갱신: Score=%.2f, Depth=%d, Actions=%d"),
			BestScore, Depth, BestPath.Num());
	}

	// 기저 조건: 깊이 한계 도달
	if (Depth >= MaxDFSDepth)
	{
		return;
	}

	// --- Branch & Bound 가지치기 (Optimization §4.2) ---
	// 남은 행동에서 얻을 수 있는 이론적 최대 점수 추정
	// Attack + Heal 양쪽의 최대 단일 점수로 상한 계산
	float MaxSingleScore = 0.0f;
	for (const auto& Pair : CachedActionScoreMap)
	{
		MaxSingleScore = FMath::Max(MaxSingleScore, Pair.Value.GetActionScore());
	}
	for (const auto& Pair : CachedHealScoreMap)
	{
		MaxSingleScore = FMath::Max(MaxSingleScore, Pair.Value.GetActionScore());
	}

	// AP + BA 양쪽 자원으로 수행 가능한 최대 행동 수 상한
	// (각 행동은 AP 또는 BA 중 하나만 소모하므로 합산이 상한)
	const int32 MaxRemainingActions = FMath::Min(
		FMath::FloorToInt32(Context.RemainingAP) + FMath::FloorToInt32(Context.RemainingBA),
		MaxDFSDepth - Depth);
	const float UpperBound = CurrentScore
		+ MaxSingleScore * MaxRemainingActions;

	if (UpperBound <= BestScore)
	{
		UE_LOG(LogPBUtility, Log,
			TEXT("[DFS] 가지치기: Depth=%d, Current=%.2f, "
				 "UpperBound=%.2f <= Best=%.2f"),
			Depth, CurrentScore, UpperBound, BestScore);
		return;
	}

	// 후보 행동 생성
	TArray<FPBSequenceAction> Candidates = GetCandidateActions(Context);
	if (Candidates.IsEmpty())
	{
		return; // 위에서 이미 BestScore 갱신 검토 완료
	}

	for (const FPBSequenceAction& Candidate : Candidates)
	{
		// 자원 소비 시도 (값 복사 후 차감 — 백트래킹 자동화)
		FPBUtilityContext BranchContext = Context;
		if (!BranchContext.TryConsumeResources(Candidate.Cost))
		{
			continue;
		}

		// Phase 3: 이동이 수반되는 행동이면 타겟 사거리 안쪽으로
		// 이동한다고 가정하고 LastActionLocation을 갱신한다.
		// (실제 좌표는 InjectMoveActions → EQS에서 최종 결정)
		if (Candidate.Cost.MovementCost > 0.0f
			&& IsValid(Candidate.TargetActor))
		{
			BranchContext.LastActionLocation =
				Candidate.TargetActor->GetActorLocation();
		}

		// 행동 점수: GetCandidateActions에서 어빌리티별 개별 점수를 캐싱
		const float ActionScore = Candidate.CachedActionScore;

		// 경로에 추가 → 재귀 탐색 → 백트래킹
		CurrentPath.Add(Candidate);
		SearchBestSequence(
			BranchContext, CurrentPath,
			CurrentScore + ActionScore,
			BestScore, BestPath, Depth + 1);
		CurrentPath.Pop();
	}
}

/*~ Fallback 위치 계산 ~*/

/*~ 적 Centroid 계산 (EQS Context + Fallback 공용) ~*/

FVector UPBUtilityClearinghouse::GetEnemyCentroid() const
{
	FVector Centroid = FVector::ZeroVector;
	int32 ValidCount = 0;

	for (const TWeakObjectPtr<AActor>& WeakTarget : CachedTargets)
	{
		if (const AActor* Target = WeakTarget.Get())
		{
			Centroid += Target->GetActorLocation();
			++ValidCount;
		}
	}

	if (ValidCount == 0)
	{
		return FVector::ZeroVector;
	}

	return Centroid / static_cast<float>(ValidCount);
}

FVector UPBUtilityClearinghouse::CalculateFallbackPosition(
	AActor *SelfRef, float RemainingMP) const
{
	if (!IsValid(SelfRef) || CachedTargets.Num() == 0)
	{
		UE_LOG(LogPBUtility, Warning,
			   TEXT("[Fallback] Self 또는 CachedTargets가 유효하지 않습니다."));
		return FVector::ZeroVector;
	}

	// 1. 적들의 평균 위치(Centroid) 계산 — 공용 헬퍼 사용
	const FVector EnemyCentroid = GetEnemyCentroid();
	if (EnemyCentroid.IsZero())
	{
		return FVector::ZeroVector;
	}

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
	UNavigationSystemV1 *NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(
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

	// Visual Logger: 후퇴 경로 시각화 (AI→후퇴 위치, 적 Centroid 표시)
	UE_VLOG_SEGMENT(SelfRef, LogPBUtility, Log,
		AIPos, ProjectedLocation.Location, FColor::Blue,
		TEXT("Fallback"));
	UE_VLOG_LOCATION(SelfRef, LogPBUtility, Log,
		EnemyCentroid, 50.0f, FColor::Orange,
		TEXT("EnemyCentroid"));

	return ProjectedLocation.Location;
}

/*~ EQS 비동기 래퍼 ~*/

void UPBUtilityClearinghouse::RunAttackPositionQuery(
	UEnvQuery* QueryAsset,
	AActor* Querier,
	AActor* TargetActor,
	FPBEQSQueryFinished OnFinished)
{
	if (!IsValid(QueryAsset) || !IsValid(Querier))
	{
		UE_LOG(LogPBUtility, Warning,
			TEXT("[EQS] RunAttackPositionQuery: "
			     "QueryAsset 또는 Querier가 유효하지 않습니다."));
		OnFinished.ExecuteIfBound(false, FVector::ZeroVector);
		return;
	}

	// Context_Target이 참조할 타겟 세팅
	EQSTargetActor = TargetActor;
	PendingAttackQueryDelegate = OnFinished;

	// EQS 쿼리 비동기 실행 (SingleResult: 최고 점수 1개만 반환)
	FEnvQueryRequest QueryRequest(QueryAsset, Querier);
	QueryRequest.Execute(
		EEnvQueryRunMode::SingleResult,
		FQueryFinishedSignature::CreateUObject(
			this, &UPBUtilityClearinghouse::HandleAttackQueryResult));

	UE_LOG(LogPBUtility, Display,
		TEXT("[EQS] AttackPosition 쿼리 실행 시작. "
		     "Querier=[%s], Target=[%s]"),
		*Querier->GetName(),
		IsValid(TargetActor) ? *TargetActor->GetName() : TEXT("None"));
}

void UPBUtilityClearinghouse::RunFallbackPositionQuery(
	UEnvQuery* QueryAsset,
	AActor* Querier,
	FPBEQSQueryFinished OnFinished)
{
	if (!IsValid(QueryAsset) || !IsValid(Querier))
	{
		UE_LOG(LogPBUtility, Warning,
			TEXT("[EQS] RunFallbackPositionQuery: "
			     "QueryAsset 또는 Querier가 유효하지 않습니다."));
		OnFinished.ExecuteIfBound(false, FVector::ZeroVector);
		return;
	}

	PendingFallbackQueryDelegate = OnFinished;

	// EQS 쿼리 비동기 실행
	// Context_EnemyCentroid는 Clearinghouse의 GetEnemyCentroid()를 자동 참조
	FEnvQueryRequest QueryRequest(QueryAsset, Querier);
	QueryRequest.Execute(
		EEnvQueryRunMode::SingleResult,
		FQueryFinishedSignature::CreateUObject(
			this, &UPBUtilityClearinghouse::HandleFallbackQueryResult));

	UE_LOG(LogPBUtility, Display,
		TEXT("[EQS] FallbackPosition 쿼리 실행 시작. Querier=[%s]"),
		*Querier->GetName());
}

/*~ EQS 쿼리 완료 핸들러 ~*/

void UPBUtilityClearinghouse::HandleAttackQueryResult(
	TSharedPtr<FEnvQueryResult> Result)
{
	FVector BestLocation = FVector::ZeroVector;
	bool bSuccess = false;

	if (Result.IsValid() && Result->IsSuccessful() && Result->Items.Num() > 0)
	{
		BestLocation = Result->GetItemAsLocation(0);
		bSuccess = true;

		UE_LOG(LogPBUtility, Display,
			TEXT("[EQS] AttackPosition 쿼리 성공. "
			     "최적 위치: (%s), Score: %.2f"),
			*BestLocation.ToCompactString(),
			Result->GetItemScore(0));
	}
	else
	{
		UE_LOG(LogPBUtility, Warning,
			TEXT("[EQS] AttackPosition 쿼리 실패 또는 결과 없음. "
			     "DFS 원래 좌표를 유지합니다."));
	}

	// 타겟 참조 정리 (다음 쿼리와 충돌 방지)
	EQSTargetActor = nullptr;

	// 콜백 실행 후 바인딩 해제
	PendingAttackQueryDelegate.ExecuteIfBound(bSuccess, BestLocation);
	PendingAttackQueryDelegate.Unbind();
}

void UPBUtilityClearinghouse::HandleFallbackQueryResult(
	TSharedPtr<FEnvQueryResult> Result)
{
	FVector BestLocation = FVector::ZeroVector;
	bool bSuccess = false;

	if (Result.IsValid() && Result->IsSuccessful() && Result->Items.Num() > 0)
	{
		BestLocation = Result->GetItemAsLocation(0);
		bSuccess = true;

		UE_LOG(LogPBUtility, Display,
			TEXT("[EQS] FallbackPosition 쿼리 성공. "
			     "최적 위치: (%s), Score: %.2f"),
			*BestLocation.ToCompactString(),
			Result->GetItemScore(0));
	}
	else
	{
		UE_LOG(LogPBUtility, Warning,
			TEXT("[EQS] FallbackPosition 쿼리 실패 또는 결과 없음. "
			     "기존 CalculateFallbackPosition 결과를 사용합니다."));
	}

	// 콜백 실행 후 바인딩 해제
	PendingFallbackQueryDelegate.ExecuteIfBound(bSuccess, BestLocation);
	PendingFallbackQueryDelegate.Unbind();
}
