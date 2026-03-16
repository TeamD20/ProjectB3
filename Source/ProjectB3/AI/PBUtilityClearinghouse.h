// PBUtilityClearinghouse.h
#pragma once

#include "CoreMinimal.h"
#include "PBAITypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "PBUtilityClearinghouse.generated.h"

// 전방 선언
class UEnvQuery;
struct FEnvQueryResult;
class UPBEnvironmentSubsystem;

// EQS 쿼리 결과 콜백 델리게이트
// bSuccess: 유효한 결과가 있는지
// ResultLocation: 쿼리가 선정한 최적 위치
DECLARE_DELEGATE_TwoParams(FPBEQSQueryFinished, bool /*bSuccess*/,
                           const FVector& /*ResultLocation*/);

// 클리어링하우스 (Context Provider)
// AI 컨트롤러가 게임 월드나 타 스탯 컴포넌트를 직접 뒤지지 않고,
// 오직 정규화(0.0 ~ 1.0)된 부드러운 판단용 지표만 뽑아갈 수 있도록 돕는
// 서브시스템.
UCLASS()
class UPBUtilityClearinghouse : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 카테고리별 ArchetypeWeight 캐시 구조체
	// ArchetypeData 미설정 시 모두 기본값 1.0 (균등 가중)
	struct FPBCachedArchetypeWeights
	{
		float AttackWeight  = 1.0f;
		float HealWeight    = 1.0f;
		float BuffWeight    = 1.0f;
		float DebuffWeight  = 1.0f;
		float ControlWeight = 1.0f;
	};

	/*~ USubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override
	{
	}

	virtual void Deinitialize() override
	{
	}

	/*~ 정규화 데이터 제공 인터페이스 ~*/

	// 활성화된 캐릭터와 타겟 사이의 지형 거리를 정규화 반환 (0.0 ~ 1.0)
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	float GetNormalizedDistanceToTarget(AActor* TargetActor) const;

	// 타겟의 현재 체력, AC 결함, 버프/디버프 상태를 종합해 취약성 퍼센티지로 반환
	// (0.0 ~ 1.0)
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	float GetTargetVulnerabilityScore(AActor* TargetActor) const;

	// 공격자의 위치가 보유한 고지대 유리함 / 엄폐 이점을 반환 (0.0 ~ 1.0)
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	float EvaluateHighGroundAdvantage(AActor* TargetActor) const;

	/*~ 스코어링 (ActionScore 산출) ~*/

	// 단일 타겟에 대한 ActionScore를 계산하여 FPBTargetScore로 반환
	// Damage_Process.md 연동 공식:
	//   ActionScore = (ExpectedDamage × TargetModifier + SituationalBonus) × ArchetypeWeight
	// ExpectedDamage는 SourceASC의 모든 활성 어빌리티 중 최고 기대 피해량
	// (CalcExpectedAttackDamage / CalcExpectedSavingThrowDamage / CalcExpectedDamage)
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	FPBTargetScore EvaluateActionScore(AActor* TargetActor);

	// 아군 1명에 대한 HealScore를 계산하여 FPBTargetScore로 반환
	// AI Scoring Example.md §3.2 공식:
	//   HealBase = EffectiveHeal × UrgencyMultiplier
	//   ActionScore = HealBase × HealWeight
	// EffectiveHeal = min(ExpectedHeal, MaxHP - CurrentHP)
	// UrgencyMultiplier: HPRatio 구간별 (≤0.25→2.0, ≤0.50→1.5, ≤0.75→1.0, >0.75→0.5)
	// ThreatMultiplier / RoleMultiplier 미적용 (아군 대상이므로)
	FPBTargetScore EvaluateHealScore(AActor* AllyTarget);

	/*~ 스코어링 (ActionScore 산출) ~*/

	// CachedTargets 전체 중 ActionScore가 가장 높은 타겟 반환
	// GenerateSequenceTask에서 CachedTargets[0] 대신 사용
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	AActor* GetBestActionScoreTarget();

	// CachedTargets를 TotalScore(ActionScore + MovementScore) 기준 내림차순으로
	// 정렬하여 상위 K개를 반환 (DFS 연산 폭발 방지 — Optimization §4.2)
	// K가 CachedTargets.Num()보다 크면 전체 반환
	TArray<FPBTargetScore> GetTopKTargets(int32 K = 3);

	/*~ DFS 다중 행동 탐색 인터페이스 ~*/

	// DFS 재귀 탐색: 현재 자원(Context)으로 가능한 최적 행동 조합을 탐색하여
	// BestPath에 기록한다. Context는 값 복사로 전달되어 백트래킹이 자동화된다.
	// GenerateSequenceTask에서 호출.
	void SearchBestSequence(
		FPBUtilityContext Context,
		TArray<FPBSequenceAction>& CurrentPath,
		float CurrentScore,
		float& BestScore,
		TArray<FPBSequenceAction>& BestPath,
		int32 Depth);

	/*~ 캐싱 (라이프사이클) 관리 인터페이스 ~*/

	/*~ 캐싱 (라이프사이클) 관리 인터페이스 ~*/

	// 특정 캐릭터의 턴이 시작되었을 때 1회만 호출하여 무거운 연산망(NavMesh,
	// Trace 등) 캐싱 반영.
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	void CacheTurnData(AActor* CurrentTurnActor);

	// AI 턴 종료 시 캐시 딕셔너리 메모리를 비운다.
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	void ClearCache();

	// AP 소진 시 방어적 후퇴 위치 계산.
	// 적 평균 위치 반대 방향으로 잔여 이동력 범위 내 NavMesh 상 위치 반환.
	// 실패 시 FVector::ZeroVector 반환 (나중에 EQS FallbackPlan으로 교체 가능).
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	FVector CalculateFallbackPosition(AActor* SelfRef, float RemainingMP) const;

	/*~ EQS 통합 인터페이스 (Phase 3) ~*/

	// EQS Context 클래스에서 참조할 현재 타겟 액터 (Attack Position 쿼리용)
	AActor* GetEQSTargetActor() const { return EQSTargetActor; }

	// 적 무리 중심 위치 반환 (EQS Context + Fallback 공용)
	// CachedTargets의 평균 위치를 계산한다.
	FVector GetEnemyCentroid() const;

	// 공격 위치 EQS 쿼리 비동기 실행
	// EQS_FindAttackPosition 에셋을 통해 타겟에 대한 최적 공격 위치를 탐색.
	// Context_Target에 TargetActor를 세팅한 뒤 쿼리를 실행하고,
	// 완료 시 OnFinished 콜백으로 결과를 전달한다.
	void RunAttackPositionQuery(
		UEnvQuery* QueryAsset,
		AActor* Querier,
		AActor* TargetActor,
		FPBEQSQueryFinished OnFinished);

	// 후퇴 위치 EQS 쿼리 비동기 실행
	// EQS_FindFallbackPosition 에셋을 통해 적 Centroid 반대 방향의
	// 엄폐 근처 최적 후퇴 위치를 탐색한다.
	void RunFallbackPositionQuery(
		UEnvQuery* QueryAsset,
		AActor* Querier,
		FPBEQSQueryFinished OnFinished);

	// EQS 쿼리 타임아웃 (초). GenerateSequenceTask에서 참조.
	static constexpr float EQSQueryTimeoutSeconds = 0.5f;

	// (테스트 편의용) 턴이 끝난 후 다시 다음 행동을 위해 자원(Action, Movement
	// 등)을 최대치로 회복시킨다.
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	void RestoreTurnResources(AActor* CurrentTurnActor);

	// 이번 턴에 인지한 아군 액터 목록 반환
	const TArray<TWeakObjectPtr<AActor>>& GetCachedAllies() const
	{
		return CachedAllies;
	}

	// 아군 대상 HealScore 캐시 맵 반환 (GenerateSequenceTask에서 Heal 후보 존재 여부 확인용)
	const TMap<AActor*, FPBTargetScore>& GetCachedHealScores() const
	{
		return CachedHealScoreMap;
	}

	// 이번 턴에 연산 대상이 될 유효 타겟 목록 반환
	const TArray<TWeakObjectPtr<AActor>>& GetCachedTargets() const
	{
		return CachedTargets;
	}

	// 적 대상 ActionScore 캐시 맵 반환 (Fallback 후 행동 탐색용)
	const TMap<AActor*, FPBTargetScore>& GetCachedActionScores() const
	{
		return CachedActionScoreMap;
	}

	// 턴 시작 시 캐싱된 최대 이동력 반환
	float GetCachedMaxMovement() const { return CachedMaxMovement; }

	// 현재 턴 수행 액터 반환 (Gameplay Debugger 등 외부 조회용)
	AActor* GetActiveTurnActor() const { return ActiveTurnActor; }

	// 캐싱된 ArchetypeWeights 반환 (디버거 표시용)
	const FPBCachedArchetypeWeights& GetCachedArchetypeWeights() const
	{
		return CachedArchetypeWeights;
	}

	// 현재 Context(잔여 자원/위치)에서 실행 가능한 후보 행동 목록 생성.
	// DFS 내부 및 Fallback 후 단일 행동 탐색에서 공용 호출.
	TArray<FPBSequenceAction> GetCandidateActions(
		const FPBUtilityContext& Context) const;

protected:
	// 이번 턴에 연산 및 판단 주체가 되는 주인공 액터.
	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveTurnActor;

	/*~ EQS 내부 상태 ~*/

	// EQS Context_Target에서 참조할 타겟 액터
	// RunAttackPositionQuery 호출 시 세팅, 쿼리 완료 시 초기화
	UPROPERTY(Transient)
	TObjectPtr<AActor> EQSTargetActor = nullptr;

	// 진행 중인 EQS 쿼리의 완료 콜백 저장소 (Attack / Fallback 각각)
	FPBEQSQueryFinished PendingAttackQueryDelegate;
	FPBEQSQueryFinished PendingFallbackQueryDelegate;

	// EQS 쿼리 완료 핸들러 (내부)
	void HandleAttackQueryResult(TSharedPtr<FEnvQueryResult> Result);
	void HandleFallbackQueryResult(TSharedPtr<FEnvQueryResult> Result);

	/*~ 캐싱 맵 자료 구조 ~*/

	// 이번 턴에 인지한 타겟 액터 목록.
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> CachedTargets;

	// 타겟 당 거리(Distance) 정규화 점수 캐시 데이터.
	UPROPERTY(Transient)
	TMap<AActor*, float> CachedDistanceMap;

	// 타겟 당 위협/취약성(Vulnerability) 정규화 점수 캐시 데이터.
	UPROPERTY(Transient)
	TMap<AActor*, float> CachedVulnerabilityMap;

	// 타겟 당 ThreatMultiplier 캐시 (CacheTurnData에서 사전 계산)
	// AI Scoring Example.md §5: lerp(0.5, 2.0, NormalizedThreat)
	TMap<AActor*, float> CachedThreatMultiplierMap;

	// 타겟(적) 당 ActionScore 평가 결과 캐시 (Attack 스코어링)
	TMap<AActor*, FPBTargetScore> CachedActionScoreMap;

	// 아군 당 HealScore 평가 결과 캐시 (Heal 스코어링)
	TMap<AActor*, FPBTargetScore> CachedHealScoreMap;

	// 이번 턴에 인지한 아군 액터 목록 (Self 포함, 사망자 제외)
	// Heal/Buff 타겟 후보로 사용
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AActor>> CachedAllies;

	FPBCachedArchetypeWeights CachedArchetypeWeights;

	// 턴 시작 시 캐싱된 최대 이동력 (Movement 실값)
	// EvaluateActionScore의 MovementScore 정규화 기준값으로 사용
	float CachedMaxMovement = 1000.0f;

	// 환경 판정 서브시스템 (LoS, 경로 비용 일원화)
	// CacheTurnData에서 1회 획득, GameInstanceSubsystem이므로 월드 독립
	UPROPERTY(Transient)
	TObjectPtr<UPBEnvironmentSubsystem> CachedEnvSubsystem = nullptr;

	/*~ 헬퍼 함수 ~*/

	// Character.Class.* 태그로부터 전투 역할 판정
	static EPBCombatRole DetermineCombatRole(AActor* TargetActor);

	// 행동 유형 × 타겟 역할 → RoleMultiplier 조회
	// AI Scoring Example.md §5 테이블 (현재 Attack 행만 구현)
	static float GetRoleMultiplier(EPBCombatRole TargetRole);


	/*~ 튜닝 상수 ~*/

	// 처치 보너스 배율 (처치 가능 시 1.0 + KillBonusRate 적용)
	float KillBonusRate = 0.5f;

	// 처치 시 제거되는 적 턴당 위협 추정값 (HP 기준 절대값)
	// TODO: ThreatScore 시스템 구현 후 실제 TargetThreatPerTurn으로 교체
	float FinishOffBaseThreat = 5.0f;

	// FinishOffBonus 잔여 라운드 최대치
	float MaxFinishOffRounds = 3.0f;

	// ThreatScore 역할별 기본 위협도
	static constexpr float RoleThreat_Healer = 5.0f;
	static constexpr float RoleThreat_Caster = 4.0f;
	static constexpr float RoleThreat_Ranged = 3.0f;
	static constexpr float RoleThreat_Melee  = 2.0f;
	static constexpr float RoleThreat_Tank   = 1.0f;

	// LowHP 축 가중치 (빈사 상태의 위협 증폭)
	static constexpr float LowHPThreatWeight = 2.0f;

	// DFS 최대 탐색 깊이 (행동 개수 상한)
	// D&D 5e 기준: Action(1) + BonusAction(1) + Move(1) = 최대 3
	// 안전 마진으로 +1하여 4로 설정
	static constexpr int32 MaxDFSDepth = 4;
};
