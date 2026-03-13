#include "PBUtilityClearinghouse.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
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
#include "ProjectB3/PBGameplayTags.h"

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

	// --- ArchetypeWeight 사전 캐싱 (1회 Cast) ---
	// APBCharacterBase로 캐스팅하여 ArchetypeData의 AttackWeight 조회.
	// 현재 Attack 행동만 구현되어 있으므로 AttackWeight만 캐싱.
	// 향후 Heal/Buff/Debuff 행동 추가 시 해당 가중치도 캐싱 확장.
	CachedArchetypeWeight = 1.0f; // 기본값 (DataAsset 미설정 시 균등 가중)
	if (const APBAIMockCharacter* AIChar = Cast<APBAIMockCharacter>(CurrentTurnActor))
	{
		if (const UPBAIArchetypeData* Archetype = AIChar->ArchetypeData)
		{
			CachedArchetypeWeight = Archetype->AttackWeight;
			UE_LOG(LogPBUtility, Log,
				TEXT("[Archetype] %s의 아키타입 캐싱 완료: AttackWeight=%.2f"),
				*CurrentTurnActor->GetName(), CachedArchetypeWeight);
		}
		else
		{
			UE_LOG(LogPBUtility, Log,
				TEXT("[Archetype] %s에 ArchetypeData 미설정 → 기본 가중치 1.0 사용"),
				*CurrentTurnActor->GetName());
		}
	}
}

void UPBUtilityClearinghouse::ClearCache()
{
	// 맵 컨테이너 요소들을 모두 비워 다음 턴이나 다른 캐릭터 연산 시 간섭이
	// 없도록 한다.
	CachedTargets.Empty();
	CachedDistanceMap.Empty();
	CachedVulnerabilityMap.Empty();
	CachedThreatMultiplierMap.Empty();
	CachedActionScoreMap.Empty();
	CachedArchetypeWeight = 1.0f;

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
	bool bBestCanKill = false;

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

			if (CandidateDamage > BestExpectedDamage)
			{
				// 어빌리티 이벤트 트리거 태그 추출 (Spec의 DynamicAbilityTags 또는 CDO AbilityTags 첫 번째)
				FGameplayTag CandidateTag;
				if (Spec.DynamicAbilityTags.Num() > 0)
				{
					CandidateTag = Spec.DynamicAbilityTags.First();
				}
				else
				{
					const FGameplayTagContainer &AbilityTags = AbilityCDO->GetAssetTags();
					if (AbilityTags.Num() > 0)
					{
						CandidateTag = AbilityTags.First();
					}
				}

				// 태그가 유효한 어빌리티만 채택 (Execute에서 HandleGameplayEvent 발동에 필수)
				if (CandidateTag.IsValid())
				{
					BestExpectedDamage = CandidateDamage;
					BestAbilityTag = CandidateTag;
					bBestCanKill = bCandidateCanKill;
				}
			}
		}
	}

	Score.ExpectedDamage = BestExpectedDamage;
	Score.AbilityTag = BestAbilityTag;

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
	// CacheTurnData에서 사전 캐싱한 값 사용 (Cast 없음)
	Score.ArchetypeWeight = CachedArchetypeWeight;

	// --- MovementScore 산정 ---
	// 공식: 1.0 - (DistToTarget / MaxMovementRange), 클램프 [0.0, 1.0]
	// 거리가 가까울수록 1.0, 멀수록 0.0
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
				"ExpDmg=%.2f (Ability=%s), TargetMod=%.2f, "
				"Situational=%.1f, Archetype=%.2f, Move=%.2f → "
				"TotalScore=%.4f"),
		   *(ActiveTurnActor ? ActiveTurnActor->GetName() : TEXT("Unknown")),
		   *TargetActor->GetName(), Score.ExpectedDamage,
		   *Score.AbilityTag.ToString(),
		   Score.TargetModifier, Score.SituationalBonus,
		   Score.ArchetypeWeight, Score.MovementScore,
		   FinalScore);

	// 결과 캐싱
	CachedActionScoreMap.Add(TargetActor, Score);
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

	// CachedActionScoreMap의 각 타겟에 대해 Attack/Move 후보 생성
	for (const auto& Pair : CachedActionScoreMap)
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

		// 어빌리티 사거리 조회 (AbilityTag 기반)
		float AbilityRange = 0.0f;
		if (ScoreData.AbilityTag.IsValid())
		{
			FGameplayTagContainer SearchTags;
			SearchTags.AddTag(ScoreData.AbilityTag);
			TArray<FGameplayAbilitySpec*> MatchingSpecs;
			SourceASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
				SearchTags, MatchingSpecs);

			if (MatchingSpecs.Num() > 0)
			{
				if (const UPBGameplayAbility_Targeted* TargetedAbility =
						Cast<UPBGameplayAbility_Targeted>(
							MatchingSpecs[0]->Ability))
				{
					AbilityRange = TargetedAbility->GetRange();
				}
			}
		}

		const float DistToTarget = FVector::Dist(
			Context.LastActionLocation, Target->GetActorLocation());
		const bool bUnlimitedRange = (AbilityRange <= 0.0f);
		constexpr float RangeBuffer = 50.0f;
		const float EffectiveRange = bUnlimitedRange
			? TNumericLimits<float>::Max()
			: AbilityRange + RangeBuffer;
		const bool bInRange = (DistToTarget <= EffectiveRange);

		// --- Attack 후보: 사거리 내 + AP 충분 ---
		if (bInRange && Context.RemainingAP >= 1.0f)
		{
			FPBSequenceAction AttackAction;
			AttackAction.ActionType = EPBActionType::Attack;
			AttackAction.TargetActor = Target;
			AttackAction.AbilityTag = ScoreData.AbilityTag;
			AttackAction.Cost.ActionCost = 1.0f;
			Candidates.Add(AttackAction);
		}

		// --- Move 후보: 사거리 밖 + 이동력 충분 ---
		if (!bInRange && !bUnlimitedRange)
		{
			const float NeededMovement = DistToTarget - EffectiveRange;
			if (NeededMovement > 0.0f && Context.CanReachTarget(Target->GetActorLocation()))
			{
				FPBSequenceAction MoveAction;
				MoveAction.ActionType = EPBActionType::Move;
				MoveAction.TargetActor = Target;
				MoveAction.Cost.MovementCost = NeededMovement;
				Candidates.Add(MoveAction);
			}
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
	}

	// 기저 조건: 깊이 한계 도달
	if (Depth >= MaxDFSDepth)
	{
		return;
	}

	// --- Branch & Bound 가지치기 (Optimization §4.2) ---
	// 남은 행동에서 얻을 수 있는 이론적 최대 점수 추정
	// Attack만 점수를 기여하므로: MaxSingleScore × 남은 공격 가능 횟수
	float MaxSingleScore = 0.0f;
	for (const auto& Pair : CachedActionScoreMap)
	{
		MaxSingleScore = FMath::Max(MaxSingleScore, Pair.Value.GetActionScore());
	}

	const int32 MaxRemainingAttacks = FMath::Min(
		FMath::FloorToInt32(Context.RemainingAP),
		MaxDFSDepth - Depth);
	const float UpperBound = CurrentScore
		+ MaxSingleScore * MaxRemainingAttacks;

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

		// Move 시 LastActionLocation 갱신 (후속 행동의 거리 계산 기준점)
		if (Candidate.ActionType == EPBActionType::Move
			&& IsValid(Candidate.TargetActor))
		{
			BranchContext.LastActionLocation =
				Candidate.TargetActor->GetActorLocation();
		}

		// 행동 점수 산출 (Attack만 점수 기여, Move는 0)
		float ActionScore = 0.0f;
		if (Candidate.ActionType == EPBActionType::Attack)
		{
			if (const FPBTargetScore* Cached =
					CachedActionScoreMap.Find(Candidate.TargetActor))
			{
				ActionScore = Cached->GetActionScore();
			}
		}

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

FVector UPBUtilityClearinghouse::CalculateFallbackPosition(
	AActor *SelfRef, float RemainingMP) const
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
	for (const TWeakObjectPtr<AActor> &WeakTarget : CachedTargets)
	{
		if (AActor *Target = WeakTarget.Get())
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

	return ProjectedLocation.Location;
}
