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
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Characters/PBEnemyCharacter.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
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
		const FPBPathFindResult PathResult = CachedEnvSubsystem->CalculatePath(
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

	APBEnemyCharacter *EnemyChar = Cast<APBEnemyCharacter>(CurrentTurnActor);
	if (!EnemyChar)
		return;

	UAbilitySystemComponent *ASC = EnemyChar->GetAbilitySystemComponent();
	const UPBTurnResourceAttributeSet *AttrSet = EnemyChar->GetTurnResourceAttributeSet();

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

	// 팩션 기반 적/아군 분류: AI의 FactionTag와 다른 팩션 → 적 타겟
	const IPBCombatParticipant* SelfParticipant = Cast<IPBCombatParticipant>(CurrentTurnActor);
	const FGameplayTag SelfFaction = SelfParticipant ? SelfParticipant->GetFactionTag() : FGameplayTag();

	TArray<AActor*> AllCharacters;
	UGameplayStatics::GetAllActorsOfClass(CurrentTurnActor->GetWorld(),
		APBCharacterBase::StaticClass(), AllCharacters);

	for (AActor* Character : AllCharacters)
	{
		if (!IsValid(Character) || Character == CurrentTurnActor)
		{
			continue;
		}

		// 사망 캐릭터 제외
		UAbilitySystemComponent* TargetASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);
		if (TargetASC && TargetASC->HasMatchingGameplayTag(
							 PBGameplayTags::Character_State_Dead))
		{
			UE_LOG(LogPBUtility, Log,
				   TEXT("사망 캐릭터 [%s]를 목록에서 제외합니다."),
				   *Character->GetName());
			continue;
		}

		// 팩션 비교
		const IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Character);
		const FGameplayTag CharFaction = Participant ? Participant->GetFactionTag() : FGameplayTag();

		if (CharFaction != SelfFaction)
		{
			CachedTargets.Add(Character);
		}
	}

	UE_LOG(LogPBUtility, Log,
		   TEXT("주변 타겟 탐색 완료. 총 %d명의 잠재적 타겟 목록 캐싱 완료."),
		   CachedTargets.Num());

	// --- ThreatScore 사전 계산 및 정규화 ---
	// ThreatMultiplier = lerp(0.5, 2.0, NormalizedThreat)
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
	if (const APBEnemyCharacter* AIChar = Cast<APBEnemyCharacter>(CurrentTurnActor))
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

	// --- 아군 캐싱 (팩션 기반) ---
	// Self를 명시적으로 아군 목록에 추가 (자가 힐/버프 후보)
	CachedAllies.Add(CurrentTurnActor);

	for (AActor* Character : AllCharacters)
	{
		if (!IsValid(Character) || Character == CurrentTurnActor)
		{
			continue;
		}

		UAbilitySystemComponent* AllyASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);
		if (AllyASC && AllyASC->HasMatchingGameplayTag(
						   PBGameplayTags::Character_State_Dead))
		{
			UE_LOG(LogPBUtility, Log,
				TEXT("사망 아군 [%s]를 아군 목록에서 제외합니다."),
				*Character->GetName());
			continue;
		}

		const IPBCombatParticipant* Participant = Cast<IPBCombatParticipant>(Character);
		const FGameplayTag CharFaction = Participant ? Participant->GetFactionTag() : FGameplayTag();

		if (CharFaction == SelfFaction)
		{
			CachedAllies.Add(Character);
		}
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
	CachedAPHealScoreMap.Empty();
	CachedBAHealScoreMap.Empty();
	CachedBuffScoreMap.Empty();
	CachedAPBuffScoreMap.Empty();
	CachedBABuffScoreMap.Empty();
	CachedDebuffScoreMap.Empty();
	CachedAPDebuffScoreMap.Empty();
	CachedBADebuffScoreMap.Empty();
	CachedControlScoreMap.Empty();
	CachedAPControlScoreMap.Empty();
	CachedBAControlScoreMap.Empty();
	CachedAllAttackScores.Empty();
	CachedAllHealScores.Empty();
	CachedAllBuffScores.Empty();
	CachedAllDebuffScores.Empty();
	CachedAllControlScores.Empty();
	CachedAoECandidates.Empty();
	CachedMultiTargetCandidates.Empty();
	CachedArchetypeWeights = FPBCachedArchetypeWeights();
	CachedMaxMovement = 1000.0f;
	EQSTargetActor = nullptr;
	EQSAbilityMaxRange = 0.f;
	EQSIdealDistance = 0.f;

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
	// 타겟 역할 가중치
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

/*~ DiceSpec 기반 기대 피해량 계산 (공용 헬퍼) ~*/

float UPBUtilityClearinghouse::CalcExpectedDamageFromDice(
	const UAbilitySystemComponent* SourceASC,
	const UAbilitySystemComponent* TargetASC,
	const FPBDiceSpec& Dice,
	const FGameplayTagContainer& SourceTags,
	const FGameplayTagContainer& TargetTags,
	float* OutRawAvg)
{
	if (!IsValid(SourceASC) || Dice.DiceCount <= 0 || Dice.DiceFaces <= 0)
	{
		if (OutRawAvg) { *OutRawAvg = 0.0f; }
		return 0.0f;
	}

	float ExpectedDamage = 0.0f;
	int32 AtkMod = 0;

	switch (Dice.RollType)
	{
	case EPBDiceRollType::HitRoll:
	{
		const int32 HitBonus = UPBAbilitySystemLibrary::GetHitBonus(
			SourceASC, Dice.BonusAttributeOverride);
		AtkMod = UPBAbilitySystemLibrary::GetAttackModifier(
			SourceASC, Dice.AttackModifierAttributeOverride);

		// HitRoll 경로: TargetASC에서 AC 조회
		int32 TargetAC = 10;
		if (IsValid(TargetASC))
		{
			bool bACFound = false;
			TargetAC = static_cast<int32>(TargetASC->GetGameplayAttributeValue(
				UPBCharacterAttributeSet::GetArmorClassAttribute(), bACFound));
		}

		ExpectedDamage = UPBAbilitySystemLibrary::CalcExpectedAttackDamage(
			Dice.DiceCount, Dice.DiceFaces, static_cast<float>(AtkMod),
			HitBonus, TargetAC, SourceTags, TargetTags);
		break;
	}

	case EPBDiceRollType::SavingThrow:
	{
		const int32 SpellDC = UPBAbilitySystemLibrary::CalcSpellSaveDC(
			SourceASC, Dice.BonusAttributeOverride);
		AtkMod = UPBAbilitySystemLibrary::GetAttackModifier(
			SourceASC, Dice.AttackModifierAttributeOverride);

		int32 SaveBonus = 0;
		if (IsValid(TargetASC))
		{
			SaveBonus = UPBAbilitySystemLibrary::GetSaveBonus(
				TargetASC, Dice.TargetSaveAttribute);
		}

		ExpectedDamage = UPBAbilitySystemLibrary::CalcExpectedSavingThrowDamage(
			Dice.DiceCount, Dice.DiceFaces, static_cast<float>(AtkMod),
			SaveBonus, SpellDC, SourceTags, TargetTags);
		break;
	}

	case EPBDiceRollType::None:
	{
		AtkMod = UPBAbilitySystemLibrary::GetAttackModifier(
			SourceASC, Dice.AttackModifierAttributeOverride);

		ExpectedDamage = UPBAbilitySystemLibrary::CalcExpectedDamage(
			Dice.DiceCount, Dice.DiceFaces, static_cast<float>(AtkMod),
			SourceTags, TargetTags);
		break;
	}
	}

	// 명중 시 평균 데미지 (확률 미반영 — KillBonus/OverhealPenalty 판정용)
	if (OutRawAvg)
	{
		*OutRawAvg = Dice.DiceCount * (Dice.DiceFaces + 1) / 2.0f
			+ static_cast<float>(AtkMod);
	}

	return ExpectedDamage;
}

/*~ StatMod 자동 HP 환산 헬퍼 ~*/

float UPBUtilityClearinghouse::GetBestRawAttackDamage(
	AActor* Attacker, AActor* Defender) const
{
	if (!IsValid(Attacker))
	{
		return 0.0f;
	}

	UAbilitySystemComponent* AttackerASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Attacker);
	if (!IsValid(AttackerASC))
	{
		return 0.0f;
	}

	UAbilitySystemComponent* DefenderASC = nullptr;
	if (IsValid(Defender))
	{
		DefenderASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Defender);
	}

	FGameplayTagContainer SourceTags, TargetTags;
	AttackerASC->GetOwnedGameplayTags(SourceTags);
	if (DefenderASC)
	{
		DefenderASC->GetOwnedGameplayTags(TargetTags);
	}

	float BestRawDmg = 0.0f;
	const TArray<FGameplayAbilitySpec>& Specs = AttackerASC->GetActivatableAbilities();
	for (const FGameplayAbilitySpec& Spec : Specs)
	{
		const UPBGameplayAbility* CDO = Cast<UPBGameplayAbility>(Spec.Ability);
		if (!CDO || CDO->GetAbilityCategory() != EPBAbilityCategory::Attack)
		{
			continue;
		}

		float RawAvg = 0.0f;
		CalcExpectedDamageFromDice(
			AttackerASC, DefenderASC,
			CDO->GetDiceSpec(), SourceTags, TargetTags,
			&RawAvg);
		BestRawDmg = FMath::Max(BestRawDmg, RawAvg);
	}

	return BestRawDmg;
}

float UPBUtilityClearinghouse::CalcAutoStatModHP(
	EPBStatModType ModType,
	float ModDelta,
	AActor* Target,
	bool bIsBuff) const
{
	if (ModType == EPBStatModType::None || FMath::IsNearlyZero(ModDelta))
	{
		return 0.0f;
	}

	const float AbsDelta = FMath::Abs(ModDelta);

	switch (ModType)
	{
	case EPBStatModType::ArmorClass:
	case EPBStatModType::AttackBonus:
	case EPBStatModType::SaveDC:
	{
		// d20 기준: ±1 = 5% 명중/내성 확률 변동
		// 경계값 클램핑: [0.05, 0.95] 범위 전체폭(0.90) 초과 방지
		const float HitChanceDelta =
			FMath::Clamp(AbsDelta * 0.05f, 0.0f, 0.90f);

		if (bIsBuff)
		{
			if (ModType == EPBStatModType::ArmorClass)
			{
				// AC 버프 → 적들이 이 아군을 덜 때림
				// 1턴 환산: Σ(적별 best raw damage) × HitChanceDelta
				float TotalEnemyRawDmg = 0.0f;
				for (const auto& EnemyWeak : CachedTargets)
				{
					AActor* Enemy = EnemyWeak.Get();
					if (!IsValid(Enemy)) { continue; }
					TotalEnemyRawDmg += GetBestRawAttackDamage(Enemy, Target);
				}
				return TotalEnemyRawDmg * HitChanceDelta;
			}
			// ATK/SaveDC 버프 → 이 아군이 적을 더 잘 때림
			AActor* RepresentativeEnemy =
				CachedTargets.Num() > 0 ? CachedTargets[0].Get() : nullptr;
			return GetBestRawAttackDamage(Target, RepresentativeEnemy)
				* HitChanceDelta;
		}

		// --- 디버프 ---
		if (ModType == EPBStatModType::ArmorClass)
		{
			// AC 디버프 → AI가 이 적을 더 잘 때림
			return GetBestRawAttackDamage(ActiveTurnActor, Target)
				* HitChanceDelta;
		}
		// ATK/SaveDC 디버프 → 이 적이 아군을 덜 때림
		return GetBestRawAttackDamage(Target, ActiveTurnActor)
			* HitChanceDelta;
	}

	case EPBStatModType::DamageBonus:
	{
		// 직접 데미지 보너스: 턴당 1회 공격 가정
		// DurationFactor는 호출부에서 별도 곱산
		return AbsDelta;
	}

	default:
		return 0.0f;
	}
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

	// Debuff 추적 (적 대상 — EstimatedHPValue × DurationFactor)
	float BestDebuff = 0.0f;
	FGameplayTag BestDebuffTag;
	FGameplayAbilitySpecHandle BestDebuffHandle;
	float BestAPDebuff = 0.0f;
	FGameplayTag BestAPDebuffTag;
	FGameplayAbilitySpecHandle BestAPDebuffHandle;
	float BestBADebuff = 0.0f;
	FGameplayTag BestBADebuffTag;
	FGameplayAbilitySpecHandle BestBADebuffHandle;

	// Control 추적 (적 대상 — EstimatedHPValue × DurationFactor)
	float BestControl = 0.0f;
	FGameplayTag BestControlTag;
	FGameplayAbilitySpecHandle BestControlHandle;
	float BestAPControl = 0.0f;
	FGameplayTag BestAPControlTag;
	FGameplayAbilitySpecHandle BestAPControlHandle;
	float BestBAControl = 0.0f;
	FGameplayTag BestBAControlTag;
	FGameplayAbilitySpecHandle BestBAControlHandle;

	// Phase 3: 모든 유효 (타겟 × 어빌리티) 조합 임시 저장
	// TargetModifier/ArchetypeWeight는 루프 이후 계산되므로 2단계 적재
	TArray<FPBTargetScore> LocalAttackScores;
	TArray<FPBTargetScore> LocalDebuffScores;
	TArray<FPBTargetScore> LocalControlScores;

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
			const UPBGameplayAbility_Targeted* TargetedCDO =
				Cast<UPBGameplayAbility_Targeted>(Spec.Ability);
			if (!TargetedCDO)
			{
				continue;
			}

			// SingleTarget만 평가 — AoE/MultiTarget은 전용 파이프라인에서 처리
			// (EvaluateAoEPlacements / EvaluateMultiTargetPlacements)
			// 여기서 스킵하지 않으면 동일 어빌리티가 DFS에 이중 등록됨
			if (TargetedCDO->GetTargetingMode() != EPBTargetingMode::SingleTarget)
			{
				continue;
			}

			// 적 대상 카테고리만 평가 (Heal/Buff는 아군 대상이므로 제외)
			const EPBAbilityCategory Category = AbilityCDO->GetAbilityCategory();
			if (Category != EPBAbilityCategory::Attack
				&& Category != EPBAbilityCategory::Debuff
				&& Category != EPBAbilityCategory::Control)
			{
				continue;
			}

			// --- Debuff/Control: EstimatedHPValue × DurationFactor 기반 스코어링 ---
			if (Category == EPBAbilityCategory::Debuff
				|| Category == EPBAbilityCategory::Control)
			{
				// 중복 체크: 타겟에 이미 같은 효과가 걸려 있으면 스킵
				const FGameplayTag EffectTag = AbilityCDO->GetEffectGrantedTag();
				if (EffectTag.IsValid()
					&& TargetASC->HasMatchingGameplayTag(EffectTag))
				{
					continue;
				}

				float HPValue = AbilityCDO->GetEstimatedHPValue();
				const int32 Duration = AbilityCDO->GetEffectDuration();

				// 수동값 미설정 + StatModType 지정 → 자동 HP 환산
				if (HPValue <= 0.0f)
				{
					const EPBStatModType ModType = AbilityCDO->GetStatModType();
					if (ModType != EPBStatModType::None)
					{
						HPValue = CalcAutoStatModHP(
							ModType, AbilityCDO->GetStatModDelta(),
							TargetActor, /*bIsBuff=*/ false);
					}
				}

				if (HPValue <= 0.0f)
				{
					continue;
				}

				const float DurationFactor =
					FMath::Min(static_cast<float>(Duration), 3.0f) / 3.0f;
				const float CandidateScore = HPValue * DurationFactor;

				const FPBCostData Cost = ExtractAbilityCost(SourceASC, Spec.Handle);
				const bool bIsBA = (Cost.BonusActionCost > 0.0f);
				const FGameplayTagContainer& Tags = AbilityCDO->GetAssetTags();
				const FGameplayTag Tag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();

				if (Category == EPBAbilityCategory::Debuff)
				{
					if (bIsBA)
					{
						if (CandidateScore > BestBADebuff)
						{
							BestBADebuff = CandidateScore;
							BestBADebuffHandle = Spec.Handle;
							BestBADebuffTag = Tag;
						}
					}
					else
					{
						if (CandidateScore > BestAPDebuff)
						{
							BestAPDebuff = CandidateScore;
							BestAPDebuffHandle = Spec.Handle;
							BestAPDebuffTag = Tag;
						}
					}
					if (CandidateScore > BestDebuff)
					{
						BestDebuff = CandidateScore;
						BestDebuffHandle = Spec.Handle;
						BestDebuffTag = Tag;
					}
				}
				else // Control
				{
					if (bIsBA)
					{
						if (CandidateScore > BestBAControl)
						{
							BestBAControl = CandidateScore;
							BestBAControlHandle = Spec.Handle;
							BestBAControlTag = Tag;
						}
					}
					else
					{
						if (CandidateScore > BestAPControl)
						{
							BestAPControl = CandidateScore;
							BestAPControlHandle = Spec.Handle;
							BestAPControlTag = Tag;
						}
					}
					if (CandidateScore > BestControl)
					{
						BestControl = CandidateScore;
						BestControlHandle = Spec.Handle;
						BestControlTag = Tag;
					}
				}

				// Phase 3: 전체 어빌리티 flat 배열 적재
				{
					FPBTargetScore Entry;
					Entry.TargetActor = TargetActor;
					Entry.ExpectedDamage = CandidateScore;
					Entry.AbilityTag = Tag;
					Entry.AbilitySpecHandle = Spec.Handle;
					(Category == EPBAbilityCategory::Debuff
						? LocalDebuffScores : LocalControlScores).Add(Entry);
				}

				continue;  // Debuff/Control은 DiceSpec 경로를 타지 않음
			}

			// --- Attack: 어빌리티 위임 기대 피해량 스코어링 ---
			const FPBDiceSpec& Dice = AbilityCDO->GetDiceSpec();

			// 데미지 주사위가 없는 어빌리티는 스킵
			if (Dice.DiceCount <= 0 || Dice.DiceFaces <= 0)
			{
				continue;
			}

			// 어빌리티의 CDO 안전 오버로드로 기대 피해량 산출
			// (ProficiencyBonus 포함, 서브클래스 오버라이드 가능)
			float RawAvgDamage = 0.0f;
			float CandidateDamage = 0.0f;
			switch (Dice.RollType)
			{
			case EPBDiceRollType::HitRoll:
				CandidateDamage = AbilityCDO->GetExpectedHitDamage(
					SourceASC, TargetASC, SourceTags, TargetTags, &RawAvgDamage);
				break;
			case EPBDiceRollType::SavingThrow:
				CandidateDamage = AbilityCDO->GetExpectedSavingThrowDamage(
					SourceASC, TargetASC, SourceTags, TargetTags, &RawAvgDamage);
				break;
			case EPBDiceRollType::None:
			default:
				CandidateDamage = AbilityCDO->GetExpectedDirectDamage(
					SourceASC, SourceTags, TargetTags, &RawAvgDamage);
				break;
			}

			// --- KillBonus / OverhealPenalty 적용 ---
			bool bCandidateCanKill = false;
			if (bHPFound && TargetCurrentHP > 0.0f)
			{
				bCandidateCanKill = (RawAvgDamage >= TargetCurrentHP);

				if (bCandidateCanKill)
				{
					const float EffectiveRatio =
						TargetCurrentHP / FMath::Max(RawAvgDamage, 1.0f);
					CandidateDamage *= EffectiveRatio * (1.0f + KillBonusRate);
				}
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

			// Phase 3: 전체 어빌리티 flat 배열 적재
			{
				FPBTargetScore Entry;
				Entry.TargetActor = TargetActor;
				Entry.ExpectedDamage = CandidateDamage;
				Entry.AbilitySpecHandle = Spec.Handle;
				Entry.SituationalBonus = bCandidateCanKill
					? FinishOffBaseThreat * MaxFinishOffRounds : 0.0f;
				const FGameplayTagContainer& Tags = AbilityCDO->GetAssetTags();
				Entry.AbilityTag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();
				LocalAttackScores.Add(Entry);
			}
		}
	}

	Score.ExpectedDamage = BestExpectedDamage;
	Score.AbilityTag = BestAbilityTag;
	Score.AbilitySpecHandle = BestSpecHandle;

	// --- TargetModifier 산정 ---
	// TargetModifier = ThreatMultiplier × RoleMultiplier
	const float* CachedThreat = CachedThreatMultiplierMap.Find(TargetActor);
	const float ThreatMultiplier = CachedThreat ? *CachedThreat : 1.0f;
	const EPBCombatRole TargetRole = DetermineCombatRole(TargetActor);
	const float RoleMultiplier = GetRoleMultiplier(TargetRole);
	Score.TargetModifier = ThreatMultiplier * RoleMultiplier;

	// --- SituationalBonus 산정 ---
	Score.SituationalBonus = 0.0f;

	// FinishOffBonus: 처치 시 제거되는 미래 위협 가치
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

	// --- Debuff 캐시 맵 갱신 ---
	// ActionScore = (BaseScore × TargetModifier) × DebuffWeight
	// BaseScore = EstimatedHPValue × DurationFactor
	auto BuildEffectScore = [&](float BestValue, const FGameplayTag& Tag,
		const FGameplayAbilitySpecHandle& Handle, float Weight) -> FPBTargetScore
	{
		FPBTargetScore S;
		S.TargetActor = TargetActor;
		S.ExpectedDamage = BestValue;  // BaseScore 재활용
		S.AbilityTag = Tag;
		S.AbilitySpecHandle = Handle;
		S.TargetModifier = Score.TargetModifier;
		S.SituationalBonus = 0.0f;
		S.ArchetypeWeight = Weight;
		S.MovementScore = Score.MovementScore;
		S.MovementWeight = Score.MovementWeight;
		return S;
	};

	if (BestDebuffHandle.IsValid())
	{
		CachedDebuffScoreMap.Add(TargetActor,
			BuildEffectScore(BestDebuff, BestDebuffTag, BestDebuffHandle,
				CachedArchetypeWeights.DebuffWeight));
	}
	if (BestAPDebuffHandle.IsValid())
	{
		CachedAPDebuffScoreMap.Add(TargetActor,
			BuildEffectScore(BestAPDebuff, BestAPDebuffTag, BestAPDebuffHandle,
				CachedArchetypeWeights.DebuffWeight));
	}
	if (BestBADebuffHandle.IsValid())
	{
		CachedBADebuffScoreMap.Add(TargetActor,
			BuildEffectScore(BestBADebuff, BestBADebuffTag, BestBADebuffHandle,
				CachedArchetypeWeights.DebuffWeight));
	}

	// --- Control 캐시 맵 갱신 ---
	if (BestControlHandle.IsValid())
	{
		CachedControlScoreMap.Add(TargetActor,
			BuildEffectScore(BestControl, BestControlTag, BestControlHandle,
				CachedArchetypeWeights.ControlWeight));
	}
	if (BestAPControlHandle.IsValid())
	{
		CachedAPControlScoreMap.Add(TargetActor,
			BuildEffectScore(BestAPControl, BestAPControlTag, BestAPControlHandle,
				CachedArchetypeWeights.ControlWeight));
	}
	if (BestBAControlHandle.IsValid())
	{
		CachedBAControlScoreMap.Add(TargetActor,
			BuildEffectScore(BestBAControl, BestBAControlTag, BestBAControlHandle,
				CachedArchetypeWeights.ControlWeight));
	}

	// --- Phase 3: 로컬 배열 → CachedAll 배열 변환 ---
	// TargetModifier·ArchetypeWeight·MovementScore를 타겟 수준 속성으로 채운다
	for (FPBTargetScore& Entry : LocalAttackScores)
	{
		Entry.TargetModifier = Score.TargetModifier;
		Entry.ArchetypeWeight = CachedArchetypeWeights.AttackWeight;
		Entry.MovementScore = Score.MovementScore;
		CachedAllAttackScores.Add(MoveTemp(Entry));
	}
	for (FPBTargetScore& Entry : LocalDebuffScores)
	{
		Entry.TargetModifier = Score.TargetModifier;
		Entry.ArchetypeWeight = CachedArchetypeWeights.DebuffWeight;
		Entry.MovementScore = Score.MovementScore;
		CachedAllDebuffScores.Add(MoveTemp(Entry));
	}
	for (FPBTargetScore& Entry : LocalControlScores)
	{
		Entry.TargetModifier = Score.TargetModifier;
		Entry.ArchetypeWeight = CachedArchetypeWeights.ControlWeight;
		Entry.MovementScore = Score.MovementScore;
		CachedAllControlScores.Add(MoveTemp(Entry));
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

	// AP/BA 자원 유형별 최고 힐 어빌리티 추적 (DFS 후보 분리용)
	float BestAPHeal = 0.0f;
	FGameplayTag BestAPHealTag;
	FGameplayAbilitySpecHandle BestAPHealHandle;

	float BestBAHeal = 0.0f;
	FGameplayTag BestBAHealTag;
	FGameplayAbilitySpecHandle BestBAHealHandle;

	// Phase 3: 모든 유효 힐 어빌리티 임시 저장 (RawExpectedHeal 기준)
	TArray<FPBTargetScore> LocalHealScores;

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

		// 어빌리티 위임 기대 회복량 산출 (CDO 안전 오버로드)
		// Heal은 판정 없음 → GetExpectedDirectDamage (태그는 향후 저항/취약 확장용)
		const FGameplayTagContainer EmptyTags;
		const float RawExpectedHeal = AbilityCDO->GetExpectedDirectDamage(
			SourceASC, EmptyTags, EmptyTags, nullptr);

		// --- 자원 유형별 최고 힐 어빌리티 추적 ---
		const FPBCostData HealAbilityCost = ExtractAbilityCost(SourceASC, Spec.Handle);
		const bool bIsBonusAction = (HealAbilityCost.BonusActionCost > 0.0f);

		if (bIsBonusAction)
		{
			if (RawExpectedHeal > BestBAHeal)
			{
				BestBAHeal = RawExpectedHeal;
				BestBAHealHandle = Spec.Handle;
				const FGameplayTagContainer& Tags = AbilityCDO->GetAssetTags();
				BestBAHealTag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();
			}
		}
		else
		{
			if (RawExpectedHeal > BestAPHeal)
			{
				BestAPHeal = RawExpectedHeal;
				BestAPHealHandle = Spec.Handle;
				const FGameplayTagContainer& Tags = AbilityCDO->GetAssetTags();
				BestAPHealTag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();
			}
		}

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

		// Phase 3: 전체 힐 어빌리티 flat 배열 적재
		// ExpectedDamage에 RawExpectedHeal을 저장, UrgencyMultiplier 등은 루프 후 적용
		{
			FPBTargetScore Entry;
			Entry.TargetActor = AllyTarget;
			Entry.ExpectedDamage = RawExpectedHeal;
			Entry.AbilitySpecHandle = Spec.Handle;
			const FGameplayTagContainer& Tags = AbilityCDO->GetAssetTags();
			Entry.AbilityTag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();
			LocalHealScores.Add(Entry);
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

	// --- UrgencyMultiplier ---
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

	// 결과 캐싱 (전체 최고 — 디버거/존재확인/B&B용)
	CachedHealScoreMap.Add(AllyTarget, Score);

	// --- 자원 유형별 캐시 맵 갱신 (DFS 후보 생성용) ---
	// UrgencyMultiplier, ArchetypeWeight는 아군 수준 속성이므로 공유
	if (BestAPHealHandle.IsValid())
	{
		const float EffectiveAPHeal = FMath::Min(BestAPHeal, MissingHP);
		FPBTargetScore APScore;
		APScore.TargetActor = AllyTarget;
		APScore.ExpectedDamage = EffectiveAPHeal;
		APScore.AbilityTag = BestAPHealTag;
		APScore.AbilitySpecHandle = BestAPHealHandle;
		APScore.TargetModifier = UrgencyMultiplier;
		APScore.SituationalBonus = 0.0f;
		APScore.ArchetypeWeight = CachedArchetypeWeights.HealWeight;
		CachedAPHealScoreMap.Add(AllyTarget, APScore);
	}
	if (BestBAHealHandle.IsValid())
	{
		const float EffectiveBAHeal = FMath::Min(BestBAHeal, MissingHP);
		FPBTargetScore BAScore;
		BAScore.TargetActor = AllyTarget;
		BAScore.ExpectedDamage = EffectiveBAHeal;
		BAScore.AbilityTag = BestBAHealTag;
		BAScore.AbilitySpecHandle = BestBAHealHandle;
		BAScore.TargetModifier = UrgencyMultiplier;
		BAScore.SituationalBonus = 0.0f;
		BAScore.ArchetypeWeight = CachedArchetypeWeights.HealWeight;
		CachedBAHealScoreMap.Add(AllyTarget, BAScore);
	}

	// --- Phase 3: 로컬 힐 배열 → CachedAllHealScores 변환 ---
	for (FPBTargetScore& Entry : LocalHealScores)
	{
		Entry.ExpectedDamage = FMath::Min(Entry.ExpectedDamage, MissingHP);
		Entry.TargetModifier = UrgencyMultiplier;
		Entry.ArchetypeWeight = CachedArchetypeWeights.HealWeight;
		CachedAllHealScores.Add(MoveTemp(Entry));
	}

	return Score;
}

/*~ Buff 스코어링 ~*/

FPBTargetScore UPBUtilityClearinghouse::EvaluateBuffScore(AActor* AllyTarget)
{
	if (!IsValid(AllyTarget))
	{
		return FPBTargetScore{};
	}

	// 캐시 먼저 확인 (턴 내 중복 연산 방지)
	if (const FPBTargetScore* Cached = CachedBuffScoreMap.Find(AllyTarget))
	{
		return *Cached;
	}

	FPBTargetScore Score;
	Score.TargetActor = AllyTarget;

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActiveTurnActor);
	if (!IsValid(SourceASC))
	{
		CachedBuffScoreMap.Add(AllyTarget, Score);
		return Score;
	}

	// 중복 체크용: 아군 타겟의 ASC
	UAbilitySystemComponent* AllyASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AllyTarget);

	float BestBuffValue = 0.0f;
	FGameplayTag BestBuffTag;
	FGameplayAbilitySpecHandle BestBuffHandle;

	// AP/BA 자원 유형별 최고 버프 어빌리티 추적 (DFS 후보 분리용)
	float BestAPBuff = 0.0f;
	FGameplayTag BestAPBuffTag;
	FGameplayAbilitySpecHandle BestAPBuffHandle;

	float BestBABuff = 0.0f;
	FGameplayTag BestBABuffTag;
	FGameplayAbilitySpecHandle BestBABuffHandle;

	// Phase 3: 모든 유효 버프 어빌리티 임시 저장
	TArray<FPBTargetScore> LocalBuffScores;

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

		// Buff 카테고리만 평가
		if (AbilityCDO->GetAbilityCategory() != EPBAbilityCategory::Buff)
		{
			continue;
		}

		// 중복 체크: 아군에 이미 같은 효과가 걸려 있으면 스킵
		const FGameplayTag EffectTag = AbilityCDO->GetEffectGrantedTag();
		if (EffectTag.IsValid() && IsValid(AllyASC)
			&& AllyASC->HasMatchingGameplayTag(EffectTag))
		{
			continue;
		}

		float HPValue = AbilityCDO->GetEstimatedHPValue();
		const int32 Duration = AbilityCDO->GetEffectDuration();

		// 수동값 미설정 + StatModType 지정 → 자동 HP 환산
		if (HPValue <= 0.0f)
		{
			const EPBStatModType ModType = AbilityCDO->GetStatModType();
			if (ModType != EPBStatModType::None)
			{
				HPValue = CalcAutoStatModHP(
					ModType, AbilityCDO->GetStatModDelta(),
					AllyTarget, /*bIsBuff=*/ true);
			}
		}

		if (HPValue <= 0.0f)
		{
			continue;
		}

		// BaseScore = EstimatedHPValue × DurationFactor
		// DurationFactor = min(EffectDuration, 3) / 3.0
		const float DurationFactor =
			FMath::Min(static_cast<float>(Duration), 3.0f) / 3.0f;
		const float CandidateScore = HPValue * DurationFactor;

		// --- 자원 유형별 최고 어빌리티 추적 ---
		const FPBCostData BuffCost = ExtractAbilityCost(SourceASC, Spec.Handle);
		const bool bIsBonusAction = (BuffCost.BonusActionCost > 0.0f);
		const FGameplayTagContainer& Tags = AbilityCDO->GetAssetTags();
		const FGameplayTag Tag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();

		if (bIsBonusAction)
		{
			if (CandidateScore > BestBABuff)
			{
				BestBABuff = CandidateScore;
				BestBABuffHandle = Spec.Handle;
				BestBABuffTag = Tag;
			}
		}
		else
		{
			if (CandidateScore > BestAPBuff)
			{
				BestAPBuff = CandidateScore;
				BestAPBuffHandle = Spec.Handle;
				BestAPBuffTag = Tag;
			}
		}

		if (CandidateScore > BestBuffValue)
		{
			BestBuffValue = CandidateScore;
			BestBuffHandle = Spec.Handle;
			BestBuffTag = Tag;
		}

		// Phase 3: 전체 버프 어빌리티 flat 배열 적재
		{
			FPBTargetScore Entry;
			Entry.TargetActor = AllyTarget;
			Entry.ExpectedDamage = CandidateScore;
			Entry.AbilitySpecHandle = Spec.Handle;
			Entry.AbilityTag = Tag;
			LocalBuffScores.Add(Entry);
		}
	}

	// Buff 어빌리티가 없으면 점수 0
	if (BestBuffValue <= 0.0f)
	{
		CachedBuffScoreMap.Add(AllyTarget, Score);
		return Score;
	}

	// --- Score 조립 ---
	// ActionScore = BaseScore × BuffWeight
	// TargetModifier = 1.0 (아군에 Threat/Role 미적용)
	Score.ExpectedDamage = BestBuffValue;  // FPBTargetScore 재활용: "기대 효과량"
	Score.AbilityTag = BestBuffTag;
	Score.AbilitySpecHandle = BestBuffHandle;
	Score.TargetModifier = 1.0f;
	Score.SituationalBonus = 0.0f;
	Score.ArchetypeWeight = CachedArchetypeWeights.BuffWeight;

	const float FinalScore = Score.GetActionScore();

	UE_LOG(LogPBUtility, Log,
		TEXT("[BuffScoring] AI [%s] → 아군 [%s]: "
			 "BaseScore=%.1f, BuffWeight=%.2f → Score=%.2f "
			 "(Ability=%s)"),
		*(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
		*AllyTarget->GetName(),
		BestBuffValue,
		CachedArchetypeWeights.BuffWeight,
		FinalScore, *BestBuffTag.ToString());

	// 결과 캐싱 (전체 최고 — 디버거/존재확인/B&B용)
	CachedBuffScoreMap.Add(AllyTarget, Score);

	// --- 자원 유형별 캐시 맵 갱신 (DFS 후보 생성용) ---
	if (BestAPBuffHandle.IsValid())
	{
		FPBTargetScore APScore;
		APScore.TargetActor = AllyTarget;
		APScore.ExpectedDamage = BestAPBuff;
		APScore.AbilityTag = BestAPBuffTag;
		APScore.AbilitySpecHandle = BestAPBuffHandle;
		APScore.TargetModifier = 1.0f;
		APScore.SituationalBonus = 0.0f;
		APScore.ArchetypeWeight = CachedArchetypeWeights.BuffWeight;
		CachedAPBuffScoreMap.Add(AllyTarget, APScore);
	}
	if (BestBABuffHandle.IsValid())
	{
		FPBTargetScore BAScore;
		BAScore.TargetActor = AllyTarget;
		BAScore.ExpectedDamage = BestBABuff;
		BAScore.AbilityTag = BestBABuffTag;
		BAScore.AbilitySpecHandle = BestBABuffHandle;
		BAScore.TargetModifier = 1.0f;
		BAScore.SituationalBonus = 0.0f;
		BAScore.ArchetypeWeight = CachedArchetypeWeights.BuffWeight;
		CachedBABuffScoreMap.Add(AllyTarget, BAScore);
	}

	// --- Phase 3: 로컬 버프 배열 → CachedAllBuffScores 변환 ---
	for (FPBTargetScore& Entry : LocalBuffScores)
	{
		Entry.TargetModifier = 1.0f;
		Entry.ArchetypeWeight = CachedArchetypeWeights.BuffWeight;
		CachedAllBuffScores.Add(MoveTemp(Entry));
	}

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

/*~ AoE 최적 배치 평가 ~*/

void UPBUtilityClearinghouse::EvaluateAoEPlacements()
{
	CachedAoECandidates.Empty();

	if (!IsValid(ActiveTurnActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActiveTurnActor);
	if (!IsValid(SourceASC))
	{
		return;
	}

	const FVector AILocation = ActiveTurnActor->GetActorLocation();

	// --- 1. AoE 어빌리티 수집 ---
	struct FAoEAbilityInfo
	{
		FGameplayAbilitySpecHandle SpecHandle;
		FGameplayTag AbilityTag;
		float AoERadius = 0.0f;
		float CastRange = 0.0f;
		FPBCostData Cost;
		EPBActionType ActionType = EPBActionType::Attack;

		// 데미지 계산에 필요한 CDO 정보
		const UPBGameplayAbility* AbilityCDO = nullptr;
	};

	TArray<FAoEAbilityInfo> AoEAbilities;

	FGameplayTagContainer SourceTags;
	SourceASC->GetOwnedGameplayTags(SourceTags);

	for (const FGameplayAbilitySpec& Spec : SourceASC->GetActivatableAbilities())
	{
		if (!Spec.Ability || !Spec.Ability->CanActivateAbility(
								Spec.Handle, SourceASC->AbilityActorInfo.Get()))
		{
			continue;
		}

		const UPBGameplayAbility_Targeted* TargetedCDO =
			Cast<UPBGameplayAbility_Targeted>(Spec.Ability);
		if (!TargetedCDO || TargetedCDO->GetTargetingMode() != EPBTargetingMode::AoE)
		{
			continue;
		}

		const UPBGameplayAbility* BaseCDO = Cast<UPBGameplayAbility>(Spec.Ability);
		if (!BaseCDO)
		{
			continue;
		}

		FAoEAbilityInfo Info;
		Info.SpecHandle = Spec.Handle;
		Info.AoERadius = TargetedCDO->GetAoERadius();
		Info.CastRange = TargetedCDO->GetRange();
		Info.Cost = ExtractAbilityCost(SourceASC, Spec.Handle);
		Info.AbilityCDO = BaseCDO;

		const FGameplayTagContainer& Tags = BaseCDO->GetAssetTags();
		Info.AbilityTag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();

		// 카테고리에 따른 ActionType 결정
		switch (BaseCDO->GetAbilityCategory())
		{
		case EPBAbilityCategory::Attack:  Info.ActionType = EPBActionType::Attack;  break;
		case EPBAbilityCategory::Debuff:  Info.ActionType = EPBActionType::Debuff;  break;
		case EPBAbilityCategory::Control: Info.ActionType = EPBActionType::Control; break;
		default: continue; // Heal/Buff AoE는 현재 미지원
		}

		AoEAbilities.Add(Info);
	}

	if (AoEAbilities.IsEmpty())
	{
		return;
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("[AoE] %d개 AoE 어빌리티 감지, %d개 적 타겟, %d개 아군"),
		AoEAbilities.Num(), CachedTargets.Num(), CachedAllies.Num());

	// --- 2. 적/아군 위치 수집 ---
	struct FActorInfo
	{
		AActor* Actor = nullptr;
		FVector Location = FVector::ZeroVector;
		bool bIsEnemy = false;
	};

	TArray<FActorInfo> EnemyInfos;
	for (const TWeakObjectPtr<AActor>& WeakTarget : CachedTargets)
	{
		if (AActor* Target = WeakTarget.Get())
		{
			// 사망 스킵
			if (const UAbilitySystemComponent* TargetASC =
					UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
			{
				if (TargetASC->HasMatchingGameplayTag(PBGameplayTags::Character_State_Dead))
				{
					continue;
				}
			}
			EnemyInfos.Add({Target, Target->GetActorLocation(), true});
		}
	}

	TArray<FActorInfo> AllyInfos;
	for (const TWeakObjectPtr<AActor>& WeakAlly : CachedAllies)
	{
		if (AActor* Ally = WeakAlly.Get())
		{
			if (Ally == ActiveTurnActor)
			{
				continue; // 자신은 별도 SelfPenalty로 처리
			}
			AllyInfos.Add({Ally, Ally->GetActorLocation(), false});
		}
	}

	if (EnemyInfos.IsEmpty())
	{
		return;
	}

	// --- 3. 후보 중심점 생성 (적 위치 + 근접 쌍 센트로이드) ---
	TArray<FVector> CandidateCenters;

	// 3-1. 각 적의 위치
	for (const FActorInfo& Enemy : EnemyInfos)
	{
		CandidateCenters.Add(Enemy.Location);
	}

	// 3-2. 근접 쌍 센트로이드 (2×AoERadius 이내 쌍)
	// 최대 반경이 어빌리티마다 다를 수 있으나, 적이 최대 4명이므로 O(N²)로 충분
	// 모든 AoE 어빌리티 중 최대 반경 기준으로 센트로이드 생성
	float MaxAoERadius = 0.0f;
	for (const FAoEAbilityInfo& Ability : AoEAbilities)
	{
		MaxAoERadius = FMath::Max(MaxAoERadius, Ability.AoERadius);
	}

	for (int32 i = 0; i < EnemyInfos.Num(); ++i)
	{
		for (int32 j = i + 1; j < EnemyInfos.Num(); ++j)
		{
			const float PairDist = FVector::Dist(
				EnemyInfos[i].Location, EnemyInfos[j].Location);
			if (PairDist <= MaxAoERadius * 2.0f)
			{
				const FVector Centroid = (EnemyInfos[i].Location + EnemyInfos[j].Location) * 0.5f;
				CandidateCenters.Add(Centroid);
			}
		}
	}

	// 3-3. 아군/자기 회피 중심점 (적 위치에서 아군·자신 반대 방향으로 오프셋)
	const int32 PreAvoidCount = CandidateCenters.Num();
	for (const FActorInfo& Enemy : EnemyInfos)
	{
		FVector AvoidDir = FVector::ZeroVector;
		int32 AvoidCount = 0;

		// 아군 회피
		for (const FActorInfo& Ally : AllyInfos)
		{
			const float Dist = FVector::DistXY(Enemy.Location, Ally.Location);
			if (Dist <= MaxAoERadius)
			{
				FVector Dir = Enemy.Location - Ally.Location;
				Dir.Z = 0.0f;
				if (!Dir.IsNearlyZero())
				{
					AvoidDir += Dir.GetSafeNormal();
					++AvoidCount;
				}
			}
		}

		// 자기 회피
		{
			const float Dist = FVector::DistXY(Enemy.Location, AILocation);
			if (Dist <= MaxAoERadius)
			{
				FVector Dir = Enemy.Location - AILocation;
				Dir.Z = 0.0f;
				if (!Dir.IsNearlyZero())
				{
					AvoidDir += Dir.GetSafeNormal();
					++AvoidCount;
				}
			}
		}

		if (AvoidCount > 0)
		{
			AvoidDir = AvoidDir.GetSafeNormal();
			const FVector ShiftedCenter = Enemy.Location + AvoidDir * (MaxAoERadius * 0.5f);
			CandidateCenters.Add(ShiftedCenter);
		}
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("[AoE] 후보 중심점 %d개 생성 (적 %d + 센트로이드 %d + 아군회피 %d)"),
		CandidateCenters.Num(), EnemyInfos.Num(),
		PreAvoidCount - EnemyInfos.Num(),
		CandidateCenters.Num() - PreAvoidCount);

	// --- 4. 어빌리티별 최적 배치 평가 ---
	for (const FAoEAbilityInfo& Ability : AoEAbilities)
	{
		FPBAoECandidate BestCandidate;
		BestCandidate.NetScore = -TNumericLimits<float>::Max();

		const float RadiusSq = Ability.AoERadius * Ability.AoERadius;

		for (const FVector& Center : CandidateCenters)
		{
			// 사거리 검증: AI 위치에서 후보 중심까지의 거리
			if (Ability.CastRange > 0.0f)
			{
				const float DistToCenter = FVector::DistXY(AILocation, Center);
				if (DistToCenter > Ability.CastRange)
				{
					continue;
				}
			}

			float ScoreSum = 0.0f;
			AActor* BestHitEnemy = nullptr;
			float BestHitEnemyScore = -1.0f;

			// 4-1. 적 피격 평가: 개별 전술 가치 합산
			for (const FActorInfo& Enemy : EnemyInfos)
			{
				const float DistSqToCenter = FVector::DistSquaredXY(Center, Enemy.Location);
				if (DistSqToCenter > RadiusSq)
				{
					continue;
				}

				// 타겟별 전술 가치 계산
				float TacticalValue = 0.0f;

				if (Ability.ActionType == EPBActionType::Attack)
				{
					// Attack: 공용 헬퍼로 기대 데미지 계산
					UAbilitySystemComponent* TargetASC =
						UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Enemy.Actor);
					if (!IsValid(TargetASC))
					{
						continue;
					}

					FGameplayTagContainer TargetTags;
					TargetASC->GetOwnedGameplayTags(TargetTags);

					const FPBDiceSpec& Dice = Ability.AbilityCDO->GetDiceSpec();
					TacticalValue = CalcExpectedDamageFromDice(
						SourceASC, TargetASC, Dice, SourceTags, TargetTags);
				}
				else
				{
					// Debuff/Control: EstimatedHPValue × DurationFactor
					const float HPValue = Ability.AbilityCDO->GetEstimatedHPValue();
					const int32 Duration = Ability.AbilityCDO->GetEffectDuration();
					const float DurationFactor =
						FMath::Min(static_cast<float>(Duration), 3.0f) / 3.0f;
					TacticalValue = HPValue * DurationFactor;
				}

				// ThreatMult × RoleMult 적용
				const float* CachedThreat = CachedThreatMultiplierMap.Find(Enemy.Actor);
				const float ThreatMult = CachedThreat ? *CachedThreat : 1.0f;
				const float RoleMult = GetRoleMultiplier(DetermineCombatRole(Enemy.Actor));
				TacticalValue *= ThreatMult * RoleMult;

				// ArchetypeWeight 적용
				switch (Ability.ActionType)
				{
				case EPBActionType::Attack:  TacticalValue *= CachedArchetypeWeights.AttackWeight;  break;
				case EPBActionType::Debuff:  TacticalValue *= CachedArchetypeWeights.DebuffWeight;  break;
				case EPBActionType::Control: TacticalValue *= CachedArchetypeWeights.ControlWeight; break;
				default: break;
				}

				ScoreSum += TacticalValue;

				// 주타겟 추적 (가장 높은 개별 점수)
				if (TacticalValue > BestHitEnemyScore)
				{
					BestHitEnemyScore = TacticalValue;
					BestHitEnemy = Enemy.Actor;
				}
			}

			// 4-2. 아군 피격 패널티 (MP 잔량 고려 — 이동으로 아군 회피 가능 여부)
			for (const FActorInfo& Ally : AllyInfos)
			{
				const float DistSqToCenter = FVector::DistSquaredXY(Center, Ally.Location);
				if (DistSqToCenter <= RadiusSq)
				{
					if (CachedMaxMovement >= Ability.AoERadius && Ability.CastRange >= Ability.AoERadius)
					{
						ScoreSum -= AoEAllyHitPenalty * 3.0f;
					}
					else
					{
						ScoreSum -= AoEAllyHitPenalty;
					}
				}
			}

			// 4-3. 자기 피격 패널티 (MP 잔량 고려)
			{
				const float SelfDistSq = FVector::DistSquaredXY(Center, AILocation);
				if (SelfDistSq <= RadiusSq)
				{
					const float SelfDistToCenter = FMath::Sqrt(SelfDistSq);
					const float EscapeDistance = Ability.AoERadius - SelfDistToCenter;
					if (CachedMaxMovement >= EscapeDistance && Ability.CastRange >= Ability.AoERadius)
					{
						// MP 충분 + 사거리 여유 → 빠져서 쏘면 됨 → 자기 포함 배치 강력 감점
						ScoreSum -= AoESelfHitPenalty * 3.0f;
					}
					else
					{
						// MP 부족 → 빠질 수 없으므로 기존 페널티
						ScoreSum -= AoESelfHitPenalty;
					}
				}
			}

			// 4-4. 최적 후보 갱신
			if (ScoreSum > BestCandidate.NetScore)
			{
				BestCandidate.Center = Center;
				BestCandidate.NetScore = ScoreSum;
				BestCandidate.AbilitySpecHandle = Ability.SpecHandle;
				BestCandidate.AbilityTag = Ability.AbilityTag;
				BestCandidate.AoERadius = Ability.AoERadius;
				BestCandidate.CastRange = Ability.CastRange;
				BestCandidate.Cost = Ability.Cost;
				BestCandidate.PrimaryTarget = BestHitEnemy;
				BestCandidate.ActionType = Ability.ActionType;
			}
		}

		// NetScore > 0 인 경우에만 후보로 등록 (자해/팀킬보다 이득이어야 함)
		if (BestCandidate.NetScore > 0.0f)
		{
			CachedAoECandidates.Add(BestCandidate);

			UE_LOG(LogPBUtility, Log,
				TEXT("[AoE] 어빌리티 [%s] 최적 배치: Center=(%s), "
					 "NetScore=%.2f, Radius=%.0f, PrimaryTarget=[%s]"),
				*Ability.AbilityTag.ToString(),
				*BestCandidate.Center.ToCompactString(),
				BestCandidate.NetScore,
				Ability.AoERadius,
				BestCandidate.PrimaryTarget
					? *BestCandidate.PrimaryTarget->GetName()
					: TEXT("None"));

			// Visual Logger: AoE 최적 배치 시각화
			UE_VLOG_LOCATION(ActiveTurnActor, LogPBUtility, Log,
				BestCandidate.Center, Ability.AoERadius, FColor::Orange,
				TEXT("AoE %s: Score=%.1f"),
				*Ability.AbilityTag.ToString(), BestCandidate.NetScore);
		}
		else
		{
			UE_LOG(LogPBUtility, Log,
				TEXT("[AoE] 어빌리티 [%s] NetScore ≤ 0 (%.2f) → 후보 탈락"),
				*Ability.AbilityTag.ToString(), BestCandidate.NetScore);
		}
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("[AoE] 최종 AoE 후보 %d개 등록"), CachedAoECandidates.Num());
}

/*~ MultiTarget 최적 분배 평가 ~*/

// 전수 열거 헬퍼: K개 발사체를 N명 타겟에 분배하는 모든 조합 생성 (Stars & Bars)
// Current[i] = 타겟 i에 할당된 발사체 수, Σ = K
// 적 4명, 발사체 3개 기준: H(4,3) = C(6,3) = 20개 — 성능 문제 없음
static void GenerateDistributions(
	int32 Remaining, int32 BinIdx, int32 NumBins,
	TArray<int32>& Current, TArray<TArray<int32>>& OutDistributions)
{
	if (BinIdx == NumBins - 1)
	{
		Current[BinIdx] = Remaining;
		OutDistributions.Add(Current);
		Current[BinIdx] = 0;
		return;
	}
	for (int32 i = 0; i <= Remaining; ++i)
	{
		Current[BinIdx] = i;
		GenerateDistributions(Remaining - i, BinIdx + 1, NumBins, Current, OutDistributions);
	}
	Current[BinIdx] = 0;
}

void UPBUtilityClearinghouse::EvaluateMultiTargetPlacements()
{
	CachedMultiTargetCandidates.Empty();

	if (!IsValid(ActiveTurnActor))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ActiveTurnActor);
	if (!IsValid(SourceASC))
	{
		return;
	}

	const FVector AILocation = ActiveTurnActor->GetActorLocation();

	FGameplayTagContainer SourceTags;
	SourceASC->GetOwnedGameplayTags(SourceTags);

	// --- 1. MultiTarget 어빌리티 수집 ---
	struct FMTAbilityInfo
	{
		FGameplayAbilitySpecHandle SpecHandle;
		FGameplayTag AbilityTag;
		int32 MaxTargetCount = 1;
		float CastRange = 0.0f;
		FPBCostData Cost;
		EPBActionType ActionType = EPBActionType::Attack;
		const UPBGameplayAbility* AbilityCDO = nullptr;
	};

	TArray<FMTAbilityInfo> MTAbilities;

	for (const FGameplayAbilitySpec& Spec : SourceASC->GetActivatableAbilities())
	{
		if (!Spec.Ability || !Spec.Ability->CanActivateAbility(
								Spec.Handle, SourceASC->AbilityActorInfo.Get()))
		{
			continue;
		}

		const UPBGameplayAbility_Targeted* TargetedCDO =
			Cast<UPBGameplayAbility_Targeted>(Spec.Ability);
		if (!TargetedCDO || TargetedCDO->GetTargetingMode() != EPBTargetingMode::MultiTarget)
		{
			continue;
		}

		const UPBGameplayAbility* BaseCDO = Cast<UPBGameplayAbility>(Spec.Ability);
		if (!BaseCDO)
		{
			continue;
		}

		FMTAbilityInfo Info;
		Info.SpecHandle = Spec.Handle;
		Info.MaxTargetCount = FMath::Max(TargetedCDO->GetMaxTargetCount(), 1);
		Info.CastRange = TargetedCDO->GetRange();
		Info.Cost = ExtractAbilityCost(SourceASC, Spec.Handle);
		Info.AbilityCDO = BaseCDO;

		const FGameplayTagContainer& Tags = BaseCDO->GetAssetTags();
		Info.AbilityTag = Tags.Num() > 0 ? Tags.First() : FGameplayTag();

		switch (BaseCDO->GetAbilityCategory())
		{
		case EPBAbilityCategory::Attack:  Info.ActionType = EPBActionType::Attack;  break;
		case EPBAbilityCategory::Debuff:  Info.ActionType = EPBActionType::Debuff;  break;
		case EPBAbilityCategory::Control: Info.ActionType = EPBActionType::Control; break;
		default: continue;
		}

		MTAbilities.Add(Info);
	}

	if (MTAbilities.IsEmpty())
	{
		return;
	}

	// --- 2. 사거리 내 유효 적 수집 (사망자 제외) ---
	struct FEnemyInfo
	{
		AActor* Actor = nullptr;
		FVector Location = FVector::ZeroVector;
		float CurrentHP = 0.0f;
	};

	TArray<FEnemyInfo> AllEnemies;
	for (const TWeakObjectPtr<AActor>& WeakTarget : CachedTargets)
	{
		AActor* Target = WeakTarget.Get();
		if (!IsValid(Target))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		if (!IsValid(TargetASC))
		{
			continue;
		}

		if (TargetASC->HasMatchingGameplayTag(PBGameplayTags::Character_State_Dead))
		{
			continue;
		}

		bool bHPFound = false;
		const float HP = TargetASC->GetGameplayAttributeValue(
			UPBCharacterAttributeSet::GetHPAttribute(), bHPFound);

		AllEnemies.Add({Target, Target->GetActorLocation(), bHPFound ? HP : 100.0f});
	}

	if (AllEnemies.IsEmpty())
	{
		return;
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("[MultiTarget] %d개 MultiTarget 어빌리티, %d명 적 타겟"),
		MTAbilities.Num(), AllEnemies.Num());

	// --- 3. 어빌리티별 분배 평가 ---
	for (const FMTAbilityInfo& Ability : MTAbilities)
	{
		// 사거리 필터링
		TArray<FEnemyInfo> ValidEnemies;
		for (const FEnemyInfo& Enemy : AllEnemies)
		{
			if (Ability.CastRange > 0.0f)
			{
				const float DistXY = FVector::DistXY(AILocation, Enemy.Location);
				if (DistXY > Ability.CastRange)
				{
					continue;
				}
			}
			ValidEnemies.Add(Enemy);
		}

		if (ValidEnemies.IsEmpty())
		{
			continue;
		}

		const int32 NumTargets = ValidEnemies.Num();
		const int32 K = Ability.MaxTargetCount;

		// --- 3-1. 타겟별 1회 피격 기대 데미지 사전 계산 ---
		TArray<float> PerHitExpected;   // 명중 확률 반영 기대값
		TArray<float> PerHitRawAvg;     // 명중 시 평균 (KillBonus/OverhealPenalty용)
		PerHitExpected.SetNum(NumTargets);
		PerHitRawAvg.SetNum(NumTargets);

		for (int32 t = 0; t < NumTargets; ++t)
		{
			UAbilitySystemComponent* TargetASC =
				UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ValidEnemies[t].Actor);
			if (!IsValid(TargetASC))
			{
				PerHitExpected[t] = 0.0f;
				PerHitRawAvg[t] = 0.0f;
				continue;
			}

			FGameplayTagContainer TargetTags;
			TargetASC->GetOwnedGameplayTags(TargetTags);

			bool bACFound = false;
			const int32 TargetAC = static_cast<int32>(TargetASC->GetGameplayAttributeValue(
				UPBCharacterAttributeSet::GetArmorClassAttribute(), bACFound));

			// 공용 헬퍼로 기대 피해량 + 명중 시 평균 데미지 계산
			const FPBDiceSpec& Dice = Ability.AbilityCDO->GetDiceSpec();
			float RawAvg = 0.0f;
			const float Expected = CalcExpectedDamageFromDice(
				SourceASC, TargetASC, Dice, SourceTags, TargetTags, &RawAvg);

			PerHitExpected[t] = Expected;
			PerHitRawAvg[t] = RawAvg;
		}

		// --- 3-2. 전수 열거 (Stars & Bars) ---
		TArray<TArray<int32>> AllDistributions;
		TArray<int32> Current;
		Current.SetNumZeroed(NumTargets);
		GenerateDistributions(K, 0, NumTargets, Current, AllDistributions);

		UE_LOG(LogPBUtility, Log,
			TEXT("[MultiTarget] 어빌리티 [%s]: %d발 × %d명 = %d개 분배 조합"),
			*Ability.AbilityTag.ToString(), K, NumTargets, AllDistributions.Num());

		// --- 3-3. 분배별 NetScore 계산 ---
		struct FScoredDistribution
		{
			TArray<int32> Hits;
			float NetScore = 0.0f;
		};

		TArray<FScoredDistribution> ScoredDistributions;

		for (const TArray<int32>& Dist : AllDistributions)
		{
			float NetScore = 0.0f;

			for (int32 t = 0; t < NumTargets; ++t)
			{
				const int32 HitCount = Dist[t];
				if (HitCount == 0)
				{
					continue;
				}

				float TotalExpected = PerHitExpected[t] * HitCount;
				const float TotalRaw = PerHitRawAvg[t] * HitCount;
				const float TargetHP = ValidEnemies[t].CurrentHP;

				// KillBonus / OverhealPenalty
				if (TargetHP > 0.0f && TotalRaw > 0.0f)
				{
					const bool bCanKill = (TotalRaw >= TargetHP);
					if (bCanKill)
					{
						const float EffectiveRatio =
							TargetHP / FMath::Max(TotalRaw, 1.0f);
						TotalExpected *= EffectiveRatio * (1.0f + KillBonusRate);
					}
				}

				// ThreatMult × RoleMult × ArchetypeWeight
				const float* CachedThreat =
					CachedThreatMultiplierMap.Find(ValidEnemies[t].Actor);
				const float ThreatMult = CachedThreat ? *CachedThreat : 1.0f;
				const float RoleMult = GetRoleMultiplier(
					DetermineCombatRole(ValidEnemies[t].Actor));
				// ActionType에 따라 올바른 ArchetypeWeight 적용 (AoE와 동일 패턴)
				float ArchWeight;
				switch (Ability.ActionType)
				{
				case EPBActionType::Debuff:  ArchWeight = CachedArchetypeWeights.DebuffWeight;  break;
				case EPBActionType::Control: ArchWeight = CachedArchetypeWeights.ControlWeight; break;
				default:                     ArchWeight = CachedArchetypeWeights.AttackWeight;   break;
				}

				NetScore += TotalExpected * ThreatMult * RoleMult * ArchWeight;
			}

			ScoredDistributions.Add({Dist, NetScore});
		}

		// --- 3-4. NetScore 내림차순 정렬 + Top-K 필터링 ---
		ScoredDistributions.Sort([](const FScoredDistribution& A, const FScoredDistribution& B)
		{
			return A.NetScore > B.NetScore;
		});

		const int32 ResultCount = FMath::Min(MultiTargetTopK, ScoredDistributions.Num());
		for (int32 i = 0; i < ResultCount; ++i)
		{
			const FScoredDistribution& Best = ScoredDistributions[i];
			if (Best.NetScore <= 0.0f)
			{
				break;
			}

			// Hit 배열 → 플랫 타겟 리스트 변환 (중복 허용)
			FPBMultiTargetCandidate Candidate;
			for (int32 t = 0; t < NumTargets; ++t)
			{
				for (int32 h = 0; h < Best.Hits[t]; ++h)
				{
					Candidate.TargetDistribution.Add(ValidEnemies[t].Actor);
				}
			}
			Candidate.NetScore = Best.NetScore;
			Candidate.AbilitySpecHandle = Ability.SpecHandle;
			Candidate.AbilityTag = Ability.AbilityTag;
			Candidate.Cost = Ability.Cost;
			Candidate.ActionType = Ability.ActionType;

			CachedMultiTargetCandidates.Add(Candidate);

			// 분배 로깅
			FString DistStr;
			for (int32 t = 0; t < NumTargets; ++t)
			{
				if (Best.Hits[t] > 0)
				{
					DistStr += FString::Printf(TEXT("%s×%d "),
						*ValidEnemies[t].Actor->GetName(), Best.Hits[t]);
				}
			}
			UE_LOG(LogPBUtility, Log,
				TEXT("[MultiTarget] 어빌리티 [%s] Top-%d: NetScore=%.2f, 분배=[%s]"),
				*Ability.AbilityTag.ToString(), i + 1,
				Best.NetScore, *DistStr.TrimEnd());
		}
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("[MultiTarget] 최종 후보 %d개 등록"), CachedMultiTargetCandidates.Num());
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

	// Phase 3: 전체 어빌리티 배열에서 공격 후보 생성
	// 같은 타겟에 대해 여러 어빌리티가 각각 후보로 등록된다.
	auto GenerateAttackCandidates = [&](const TArray<FPBTargetScore>& AllScores)
	{
		for (const FPBTargetScore& ScoreData : AllScores)
		{
			AActor* Target = ScoreData.TargetActor;
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
				const bool bOutOfRange = !bInRange && !bUnlimitedRange;
				const bool bNeedsMovement = bOutOfRange || !bHasLoS;

				if (bNeedsMovement)
				{
					const float AvailableMP = Context.RemainingMP - Context.AccumulatedMP;

					if (bOutOfRange)
					{
						// 사거리 도달 비용 — 직선 거리 기반 추정
						const float NeededMovement = FMath::Max(
							DistToTargetXY - AbilityRange, 1.0f);

						if (NeededMovement > AvailableMP)
						{
							continue; // MP가 절대적으로 부족 → 후보 제외
						}

						// CanReachTarget 성공 시 정확한 비용, 실패 시 남은 MP 전체 사용
						// (벽 뒤라 직선 도달 불가해도 EQS가 우회 위치를 찾아줌)
						if (Context.CanReachTarget(Target->GetActorLocation()))
						{
							AttackAction.Cost.MovementCost = NeededMovement;
						}
						else
						{
							AttackAction.Cost.MovementCost = AvailableMP;
						}
					}
					else
					{
						// 사거리 내지만 LoS 차단 → 벽 우회 재배치 비용 (휴리스틱)
						// 실제 위치는 EQS가 LoS + 사거리 기준으로 결정
						const float AvailableMPForLoS = Context.RemainingMP - Context.AccumulatedMP;
						if (AvailableMPForLoS <= 0.f)
						{
							continue;  // MP 없어서 LoS 확보 위치로 이동 불가
						}
						static constexpr float LoSRepositionEstimate = 100.0f;
						AttackAction.Cost.MovementCost = FMath::Min(LoSRepositionEstimate, AvailableMPForLoS);
					}
				}

				Candidates.Add(AttackAction);
			}
		}
	};

	// Phase 3: 전체 어빌리티 배열에서 공격 후보 생성 (타겟×어빌리티 모든 조합)
	GenerateAttackCandidates(CachedAllAttackScores);

	// --- Heal/Buff 후보: 전체 어빌리티 배열에서 아군 대상 ---
	auto GenerateHealCandidates = [&](const TArray<FPBTargetScore>& AllScores,
		EPBActionType OverrideActionType = EPBActionType::Heal)
	{
		for (const FPBTargetScore& HealData : AllScores)
		{
			AActor* Ally = HealData.TargetActor;

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
				HealAction.ActionType = OverrideActionType;
				HealAction.TargetActor = Ally;
				HealAction.AbilityTag = HealData.AbilityTag;
				HealAction.AbilitySpecHandle = HealData.AbilitySpecHandle;
				HealAction.Cost.ActionCost = HealCost.ActionCost;
				HealAction.Cost.BonusActionCost = HealCost.BonusActionCost;
				HealAction.CachedActionScore = HealData.GetActionScore();

				const bool bHealOutOfRange = !bHealInRange && !bHealUnlimited;
				const bool bHealNeedsMovement = bHealOutOfRange || !bHealHasLoS;

				if (bHealNeedsMovement)
				{
					if (bHealOutOfRange)
					{
						const float NeededMovement = FMath::Max(
							DistToAllyXY - HealRange, 1.0f);
						const float AvailableMP = Context.RemainingMP - Context.AccumulatedMP;
						if (NeededMovement > AvailableMP)
						{
							continue;  // MP가 절대적으로 부족
						}
						if (Context.CanReachTarget(Ally->GetActorLocation()))
						{
							HealAction.Cost.MovementCost = NeededMovement;
						}
						else
						{
							HealAction.Cost.MovementCost = AvailableMP;  // EQS가 우회 위치 찾아줌
						}
					}
					else
					{
						const float AvailableMPForLoS = Context.RemainingMP - Context.AccumulatedMP;
						if (AvailableMPForLoS <= 0.f)
						{
							continue;  // MP 없어서 LoS 확보 위치로 이동 불가
						}
						static constexpr float LoSRepositionEstimate = 100.0f;
						HealAction.Cost.MovementCost = FMath::Min(LoSRepositionEstimate, AvailableMPForLoS);
					}
				}

				Candidates.Add(HealAction);
			}
		}
	};

	// Phase 3: 전체 힐 어빌리티 배열에서 후보 생성
	GenerateHealCandidates(CachedAllHealScores);

	// --- Debuff/Control 후보: 전체 어빌리티 배열에서 적 대상 ---
	auto GenerateEnemyEffectCandidates = [&](
		const TArray<FPBTargetScore>& AllScores, EPBActionType ActionType)
	{
		for (const FPBTargetScore& ScoreData : AllScores)
		{
			AActor* Target = ScoreData.TargetActor;

			if (!IsValid(Target))
			{
				continue;
			}

			if (ScoreData.GetActionScore() <= 0.0f)
			{
				continue;
			}

			const FPBCostData Cost = ExtractAbilityCost(SourceASC, ScoreData.AbilitySpecHandle);
			if (Context.RemainingAP < Cost.ActionCost
				|| Context.RemainingBA < Cost.BonusActionCost)
			{
				continue;
			}

			// 사거리 검사 (SpecHandle 기반)
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

			const float DistXY = FVector::DistXY(
				Context.LastActionLocation, Target->GetActorLocation());
			const bool bUnlimited = (AbilityRange <= 0.0f);
			const bool bInRange = bUnlimited || (DistXY <= AbilityRange);

			bool bHasLoS = true;
			if (CachedEnvSubsystem)
			{
				const FPBLoSResult LoS = CachedEnvSubsystem->CheckLineOfSight(
					Context.LastActionLocation, Target);
				bHasLoS = LoS.bHasLineOfSight;
			}

			FPBSequenceAction Action;
			Action.ActionType = ActionType;
			Action.TargetActor = Target;
			Action.AbilityTag = ScoreData.AbilityTag;
			Action.AbilitySpecHandle = ScoreData.AbilitySpecHandle;
			Action.Cost.ActionCost = Cost.ActionCost;
			Action.Cost.BonusActionCost = Cost.BonusActionCost;
			Action.CachedActionScore = ScoreData.GetActionScore();

			const bool bEffectOutOfRange = !bInRange && !bUnlimited;
			const bool bNeedsMove = bEffectOutOfRange || !bHasLoS;
			if (bNeedsMove)
			{
				if (bEffectOutOfRange)
				{
					const float Needed = FMath::Max(DistXY - AbilityRange, 1.0f);
					const float AvailableMP = Context.RemainingMP - Context.AccumulatedMP;
					if (Needed > AvailableMP)
					{
						continue;  // MP가 절대적으로 부족
					}
					if (Context.CanReachTarget(Target->GetActorLocation()))
					{
						Action.Cost.MovementCost = Needed;
					}
					else
					{
						Action.Cost.MovementCost = AvailableMP;  // EQS가 우회 위치 찾아줌
					}
				}
				else
				{
					const float AvailableMPForLoS = Context.RemainingMP - Context.AccumulatedMP;
					if (AvailableMPForLoS <= 0.f)
					{
						continue;  // MP 없어서 LoS 확보 위치로 이동 불가
					}
					static constexpr float LoSRepositionEstimate = 100.0f;
					Action.Cost.MovementCost = FMath::Min(LoSRepositionEstimate, AvailableMPForLoS);
				}
			}

			Candidates.Add(Action);
		}
	};

	// Phase 3: 전체 어빌리티 배열에서 Debuff/Control/Buff 후보 생성
	GenerateEnemyEffectCandidates(CachedAllDebuffScores, EPBActionType::Debuff);
	GenerateEnemyEffectCandidates(CachedAllControlScores, EPBActionType::Control);
	GenerateHealCandidates(CachedAllBuffScores, EPBActionType::Buff);

	// --- AoE 후보: CachedAoECandidates에서 후보 생성 ---
	for (const FPBAoECandidate& AoECandidate : CachedAoECandidates)
	{
		// 자원 검사
		if (Context.RemainingAP < AoECandidate.Cost.ActionCost
			|| Context.RemainingBA < AoECandidate.Cost.BonusActionCost)
		{
			continue;
		}

		// 사거리 검사: 현재 위치에서 AoE 중심까지 도달 가능한지
		const float DistToCenter = FVector::DistXY(
			Context.LastActionLocation, AoECandidate.Center);
		const bool bUnlimitedRange = (AoECandidate.CastRange <= 0.0f);
		const bool bInRange = bUnlimitedRange || (DistToCenter <= AoECandidate.CastRange);

		// LoS 판정: 현재 위치에서 AoE 주타겟까지 시야 확보 여부
		bool bAoEHasLoS = true;
		if (CachedEnvSubsystem && IsValid(AoECandidate.PrimaryTarget))
		{
			const FPBLoSResult LoSResult = CachedEnvSubsystem->CheckLineOfSight(
				Context.LastActionLocation, AoECandidate.PrimaryTarget);
			bAoEHasLoS = LoSResult.bHasLineOfSight;
		}

		FPBSequenceAction AoEAction;
		AoEAction.ActionType = AoECandidate.ActionType;
		AoEAction.TargetActor = AoECandidate.PrimaryTarget;
		AoEAction.TargetLocation = AoECandidate.Center;
		AoEAction.AbilityTag = AoECandidate.AbilityTag;
		AoEAction.AbilitySpecHandle = AoECandidate.AbilitySpecHandle;
		AoEAction.Cost.ActionCost = AoECandidate.Cost.ActionCost;
		AoEAction.Cost.BonusActionCost = AoECandidate.Cost.BonusActionCost;
		AoEAction.CachedActionScore = AoECandidate.NetScore;

		const bool bAoEOutOfRange = !bInRange && !bUnlimitedRange;
		const bool bAoENeedsMovement = bAoEOutOfRange || !bAoEHasLoS;

		if (bAoENeedsMovement)
		{
			if (bAoEOutOfRange)
			{
				const float NeededMovement = FMath::Max(
					DistToCenter - AoECandidate.CastRange, 1.0f);
				if (Context.CanReachTarget(AoECandidate.Center))
				{
					AoEAction.Cost.MovementCost = NeededMovement;
				}
				else
				{
					continue; // 도달 불가
				}
			}
			else
			{
				// 사거리 내지만 LoS 차단 → 벽 우회 재배치 비용 (휴리스틱)
				const float AvailableMPForLoS = Context.RemainingMP - Context.AccumulatedMP;
				if (AvailableMPForLoS <= 0.f)
				{
					continue;  // MP 없어서 LoS 확보 위치로 이동 불가
				}
				static constexpr float LoSRepositionEstimate = 100.0f;
				AoEAction.Cost.MovementCost = FMath::Min(LoSRepositionEstimate, AvailableMPForLoS);
			}
		}

		Candidates.Add(AoEAction);
	}

	// --- MultiTarget 후보: CachedMultiTargetCandidates에서 후보 생성 ---
	for (const FPBMultiTargetCandidate& MTCandidate : CachedMultiTargetCandidates)
	{
		// 자원 검사
		if (Context.RemainingAP < MTCandidate.Cost.ActionCost
			|| Context.RemainingBA < MTCandidate.Cost.BonusActionCost)
		{
			continue;
		}

		// 사거리 검사: 모든 타겟이 현재 위치에서 사거리 내인지 확인
		// (EvaluateMultiTargetPlacements에서 AI 위치 기준으로 검사했지만,
		//  DFS 중 이동 후 위치가 달라질 수 있으므로 재검증)
		bool bAllInRange = true;
		float MaxNeededMovement = 0.0f;

		// 어빌리티 사거리 조회
		float MTRange = 0.0f;
		if (MTCandidate.AbilitySpecHandle.IsValid())
		{
			if (const FGameplayAbilitySpec* FoundSpec =
					SourceASC->FindAbilitySpecFromHandle(MTCandidate.AbilitySpecHandle))
			{
				if (const UPBGameplayAbility_Targeted* TargetedAbility =
						Cast<UPBGameplayAbility_Targeted>(FoundSpec->Ability))
				{
					MTRange = TargetedAbility->GetRange();
				}
			}
		}

		// 중복 제거된 유니크 타겟 세트로 사거리 검사
		TSet<AActor*> UniqueTargets;
		for (const TObjectPtr<AActor>& Target : MTCandidate.TargetDistribution)
		{
			UniqueTargets.Add(Target.Get());
		}

		bool bMTHasLoS = true;
		for (AActor* Target : UniqueTargets)
		{
			if (!IsValid(Target))
			{
				bAllInRange = false;
				break;
			}

			// LoS 판정: 현재 위치에서 각 타겟까지 시야 확보 여부
			if (CachedEnvSubsystem)
			{
				const FPBLoSResult LoSResult = CachedEnvSubsystem->CheckLineOfSight(
					Context.LastActionLocation, Target);
				if (!LoSResult.bHasLineOfSight)
				{
					bMTHasLoS = false;
				}
			}

			const float DistXY = FVector::DistXY(
				Context.LastActionLocation, Target->GetActorLocation());
			const bool bUnlimited = (MTRange <= 0.0f);

			if (!bUnlimited && DistXY > MTRange)
			{
				const float Needed = FMath::Max(DistXY - MTRange, 1.0f);
				MaxNeededMovement = FMath::Max(MaxNeededMovement, Needed);
			}
		}

		if (!bAllInRange)
		{
			continue;
		}

		FPBSequenceAction MTAction;
		MTAction.ActionType = MTCandidate.ActionType;
		MTAction.TargetActor = MTCandidate.TargetDistribution.Num() > 0
			? MTCandidate.TargetDistribution[0].Get() : nullptr;
		MTAction.AbilityTag = MTCandidate.AbilityTag;
		MTAction.AbilitySpecHandle = MTCandidate.AbilitySpecHandle;
		MTAction.Cost.ActionCost = MTCandidate.Cost.ActionCost;
		MTAction.Cost.BonusActionCost = MTCandidate.Cost.BonusActionCost;
		MTAction.CachedActionScore = MTCandidate.NetScore;

		// MultiTarget 분배 리스트 복사
		MTAction.MultiTargetActors = MTCandidate.TargetDistribution;

		// 이동 필요 시 MovementCost 부착
		const bool bMTNeedsMovement = (MaxNeededMovement > 0.0f) || !bMTHasLoS;
		if (bMTNeedsMovement)
		{
			if (MaxNeededMovement > 0.0f)
			{
				// 사거리 도달 비용
				if (MTAction.TargetActor && Context.CanReachTarget(
						MTAction.TargetActor->GetActorLocation()))
				{
					MTAction.Cost.MovementCost = MaxNeededMovement;
				}
				else
				{
					continue; // 도달 불가
				}
			}
			else
			{
				// 사거리 내지만 LoS 차단 → 벽 우회 재배치 비용
				const float AvailableMPForLoS = Context.RemainingMP - Context.AccumulatedMP;
				if (AvailableMPForLoS <= 0.f)
				{
					continue;  // MP 없어서 LoS 확보 위치로 이동 불가
				}
				static constexpr float LoSRepositionEstimate = 100.0f;
				MTAction.Cost.MovementCost = FMath::Min(LoSRepositionEstimate, AvailableMPForLoS);
			}
		}

		Candidates.Add(MTAction);
	}

	UE_LOG(LogPBUtility, Log,
		TEXT("[DFS] GetCandidateActions: %d개 후보 행동 생성 "
			 "(AP=%.0f, BA=%.0f, MP잔여=%.0f, 누적MP=%.0f)"),
		Candidates.Num(),
		Context.RemainingAP, Context.RemainingBA,
		Context.RemainingMP, Context.AccumulatedMP);

	return Candidates;
}

/*~ B&B 가지치기용 단일 행동 최대 점수 사전 계산 ~*/

float UPBUtilityClearinghouse::CalcMaxSingleScore() const
{
	float MaxScore = 0.0f;

	// 5개 카테고리 캐시 맵 순회
	auto UpdateMax = [&](const TMap<AActor*, FPBTargetScore>& Map)
	{
		for (const auto& Pair : Map)
		{
			MaxScore = FMath::Max(MaxScore, Pair.Value.GetActionScore());
		}
	};
	UpdateMax(CachedActionScoreMap);
	UpdateMax(CachedHealScoreMap);
	UpdateMax(CachedBuffScoreMap);
	UpdateMax(CachedDebuffScoreMap);
	UpdateMax(CachedControlScoreMap);

	// AoE NetScore는 다중 타겟 합산이므로 단일 타겟보다 높을 수 있음
	for (const FPBAoECandidate& AoE : CachedAoECandidates)
	{
		MaxScore = FMath::Max(MaxScore, AoE.NetScore);
	}

	// MultiTarget NetScore도 다중 발사체 합산이므로 단일 타겟을 초과할 수 있음
	for (const FPBMultiTargetCandidate& MT : CachedMultiTargetCandidates)
	{
		MaxScore = FMath::Max(MaxScore, MT.NetScore);
	}

	return MaxScore;
}

/*~ DFS 다중 행동 탐색 ~*/

void UPBUtilityClearinghouse::SearchBestSequence(
	FPBUtilityContext Context,
	TArray<FPBSequenceAction>& CurrentPath,
	float CurrentScore,
	float& BestScore,
	TArray<FPBSequenceAction>& BestPath,
	int32 Depth,
	float MaxSingleScore)
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

	// --- Branch & Bound 가지치기 ---
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
			BestScore, BestPath, Depth + 1,
			MaxSingleScore);
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

	const FVector AIPos = SelfRef->GetActorLocation();

	// 2. 가장 가까운 적까지의 2D 거리 계산
	float NearestEnemyDistXY = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<AActor>& TargetWeak : CachedTargets)
	{
		if (const AActor* Target = TargetWeak.Get())
		{
			const float DistXY = FVector::DistXY(AIPos, Target->GetActorLocation());
			NearestEnemyDistXY = FMath::Min(NearestEnemyDistXY, DistXY);
		}
	}

	// 3. ASC에서 보유 Targeted 어빌리티의 사거리 분석
	//    - 유한 사거리(Range > 0): 카이트 거리 기준으로 사용
	//    - 무한 사거리(Range == 0): 어디서든 공격 가능 → 안전 거리 기반 후퇴
	float BestFiniteRange = 0.f;
	bool bHasUnlimitedRange = false;
	bool bHasTargetedAbility = false;

	if (const UAbilitySystemComponent* ASC =
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelfRef))
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (const UPBGameplayAbility_Targeted* TargetedCDO =
					Cast<UPBGameplayAbility_Targeted>(Spec.Ability))
			{
				bHasTargetedAbility = true;
				const float Range = TargetedCDO->GetRange();
				if (Range > 0.f)
				{
					BestFiniteRange = FMath::Max(BestFiniteRange, Range);
				}
				else
				{
					bHasUnlimitedRange = true;
				}
			}
		}
	}

	// 4. 전술적 카이트 거리 계산
	//    - 유한 사거리 보유: 사거리 85% 지점 유지 (경계 안쪽 안전 마진)
	//    - 무한 사거리 전용: 이동력 50%만 후퇴 (안전 확보 + 다음 턴 이동 여유)
	//    - Targeted 어빌리티 없음 (근접 전용): 기본 500cm
	static constexpr float KiteRangeRatio = 0.85f;
	static constexpr float MeleeDefaultKiteDistance = 500.0f;
	static constexpr float UnlimitedRangeSafetyRatio = 0.5f;

	float DesiredDistFromEnemy;
	if (BestFiniteRange > 0.f)
	{
		// 유한 사거리 어빌리티 있음 → 사거리 경계 안쪽
		DesiredDistFromEnemy = BestFiniteRange * KiteRangeRatio;
	}
	else if (bHasUnlimitedRange)
	{
		// 무한 사거리 전용 → 어디서든 공격 가능하지만 무한 후퇴 방지
		// 현재 거리가 이미 충분하면 유지, 아니면 최대 1500cm까지만 후퇴
		static constexpr float UnlimitedMaxKiteDistance = 1500.0f;
		DesiredDistFromEnemy = FMath::Min(
			NearestEnemyDistXY + RemainingMP * UnlimitedRangeSafetyRatio,
			UnlimitedMaxKiteDistance);
	}
	else
	{
		// Targeted 어빌리티 없음 (근접 전용)
		DesiredDistFromEnemy = MeleeDefaultKiteDistance;
	}

	// 이미 원하는 거리 이상이면 후퇴 불필요 → 제자리 유지
	const float NeededRetreat = FMath::Max(0.f, DesiredDistFromEnemy - NearestEnemyDistXY);
	const float ActualRetreat = FMath::Min(NeededRetreat, RemainingMP);

	if (ActualRetreat <= 0.f)
	{
		// 후퇴 불필요 — 적이 공격 사거리 밖이면 접근 시도
		const bool bEnemyOutOfRange =
			(BestFiniteRange > 0.f && NearestEnemyDistXY > BestFiniteRange);

		if (!bEnemyOutOfRange)
		{
			UE_LOG(LogPBUtility, Display,
				TEXT("[Fallback] 이미 카이트 거리(%.0f) 이상, 사거리 내. "
					 "NearestEnemy=%.0f, BestFiniteRange=%.0f, bUnlimited=%d"),
				DesiredDistFromEnemy, NearestEnemyDistXY,
				BestFiniteRange, bHasUnlimitedRange);
			return FVector::ZeroVector;
		}

		// 사거리 경계 안쪽(KiteRangeRatio)까지 접근
		const float DesiredApproachDist =
			NearestEnemyDistXY - BestFiniteRange * KiteRangeRatio;
		const float ActualApproach = FMath::Min(DesiredApproachDist, RemainingMP);

		FVector ApproachDir = EnemyCentroid - AIPos;
		ApproachDir.Z = 0.0f;
		if (ApproachDir.IsNearlyZero())
		{
			ApproachDir = SelfRef->GetActorForwardVector();
		}
		ApproachDir.Normalize();

		const FVector ApproachCandidate = AIPos + ApproachDir * ActualApproach;

		// NavMesh 프로젝션
		UNavigationSystemV1* ApproachNavSys =
			FNavigationSystem::GetCurrent<UNavigationSystemV1>(SelfRef->GetWorld());
		if (!ApproachNavSys)
		{
			UE_LOG(LogPBUtility, Warning,
				TEXT("[Fallback] NavigationSystem을 찾을 수 없습니다 (접근)."));
			return FVector::ZeroVector;
		}

		FNavLocation ApproachProjected;
		bool bApproachProjected = ApproachNavSys->ProjectPointToNavigation(
			ApproachCandidate, ApproachProjected, FVector(200.0f, 200.0f, 200.0f));

		if (!bApproachProjected)
		{
			const FVector HalfApproach = AIPos + ApproachDir * (ActualApproach * 0.5f);
			bApproachProjected = ApproachNavSys->ProjectPointToNavigation(
				HalfApproach, ApproachProjected, FVector(200.0f, 200.0f, 200.0f));

			if (!bApproachProjected)
			{
				UE_LOG(LogPBUtility, Warning,
					TEXT("[Fallback] NavMesh 투영 실패. 접근 불가."));
				return FVector::ZeroVector;
			}
		}

		UE_LOG(LogPBUtility, Display,
			TEXT("[Fallback] 사거리 밖 접근: (%s) → (%s) | "
				 "BestFiniteRange=%.0f, NearestEnemy=%.0f, "
				 "DesiredApproach=%.0f, ActualApproach=%.0f, MP=%.0f"),
			*AIPos.ToCompactString(),
			*ApproachProjected.Location.ToCompactString(),
			BestFiniteRange, NearestEnemyDistXY,
			DesiredApproachDist, ActualApproach, RemainingMP);

		UE_VLOG_SEGMENT(SelfRef, LogPBUtility, Log,
			AIPos, ApproachProjected.Location, FColor::Green,
			TEXT("Approach"));

		return ApproachProjected.Location;
	}

	// 5. 적 Centroid 반대 방향 벡터 (XY 평면)
	FVector RetreatDir = AIPos - EnemyCentroid;
	RetreatDir.Z = 0.0f;

	if (RetreatDir.IsNearlyZero())
	{
		RetreatDir = SelfRef->GetActorForwardVector();
	}

	RetreatDir.Normalize();

	// 6. 클램프된 전술 거리만큼만 후퇴
	const FVector CandidatePos = AIPos + RetreatDir * ActualRetreat;

	// 7. NavMesh 프로젝션으로 도달 가능 위치 보정
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
		const FVector HalfCandidate = AIPos + RetreatDir * (ActualRetreat * 0.5f);
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
		   TEXT("[Fallback] 전술 후퇴: (%s) → (%s) | "
				"BestFiniteRange=%.0f, bUnlimited=%d, DesiredDist=%.0f, "
				"NearestEnemy=%.0f, Needed=%.0f, Actual=%.0f, MP=%.0f"),
		   *AIPos.ToCompactString(),
		   *ProjectedLocation.Location.ToCompactString(),
		   BestFiniteRange, bHasUnlimitedRange, DesiredDistFromEnemy,
		   NearestEnemyDistXY, NeededRetreat, ActualRetreat, RemainingMP);

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
	float AbilityMaxRange,
	float IdealDistance,
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

	// Context_Target이 참조할 타겟 + AbilityRange/IdealDistance 테스트가 참조할 값 세팅
	EQSTargetActor = TargetActor;
	EQSAbilityMaxRange = AbilityMaxRange;
	EQSIdealDistance = IdealDistance;
	PendingAttackQueryDelegate = OnFinished;

	// EQS 쿼리 비동기 실행 (SingleResult: 최고 점수 1개만 반환)
	FEnvQueryRequest QueryRequest(QueryAsset, Querier);
	QueryRequest.Execute(
		EEnvQueryRunMode::SingleResult,
		FQueryFinishedSignature::CreateUObject(
			this, &UPBUtilityClearinghouse::HandleAttackQueryResult));

	UE_LOG(LogPBUtility, Display,
		TEXT("[EQS] AttackPosition 쿼리 실행 시작. "
		     "Querier=[%s], Target=[%s], MaxRange=%.0f%s, IdealDist=%.0f"),
		*Querier->GetName(),
		IsValid(TargetActor) ? *TargetActor->GetName() : TEXT("None"),
		AbilityMaxRange,
		AbilityMaxRange <= 0.f ? TEXT(" (무제한)") : TEXT(""),
		IdealDistance);
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

	// 타겟 + 사거리 + 이상거리 참조 정리 (다음 쿼리와 충돌 방지)
	EQSTargetActor = nullptr;
	EQSAbilityMaxRange = 0.f;
	EQSIdealDistance = 0.f;

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
