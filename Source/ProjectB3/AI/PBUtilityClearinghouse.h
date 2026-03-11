// PBUtilityClearinghouse.h
#pragma once

#include "CoreMinimal.h"
#include "PBAITypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "PBUtilityClearinghouse.generated.h"

// 클리어링하우스 (Context Provider)
// AI 컨트롤러가 게임 월드나 타 스탯 컴포넌트를 직접 뒤지지 않고,
// 오직 정규화(0.0 ~ 1.0)된 부드러운 판단용 지표만 뽑아갈 수 있도록 돕는
// 서브시스템.
UCLASS()
class UPBUtilityClearinghouse : public UWorldSubsystem
{
	GENERATED_BODY()

public:
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
	// AI Scoring Example.md: (BaseScore × HitProb × TargetModifier + Situational) × Archetype
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	FPBTargetScore EvaluateActionScore(AActor* TargetActor);

	/*~ 스코어링 (ActionScore 산출) ~*/

	// CachedTargets 전체 중 ActionScore가 가장 높은 타겟 반환
	// GenerateSequenceTask에서 CachedTargets[0] 대신 사용
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	AActor* GetBestActionScoreTarget();

	// CachedTargets를 TotalScore(ActionScore + MovementScore) 기준 내림차순으로
	// 정렬하여 상위 K개를 반환 (DFS 연산 폭발 방지 — Optimization §4.2)
	// K가 CachedTargets.Num()보다 크면 전체 반환
	TArray<FPBTargetScore> GetTopKTargets(int32 K = 3);

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

	// (테스트 편의용) 턴이 끝난 후 다시 다음 행동을 위해 자원(Action, Movement
	// 등)을 최대치로 회복시킨다.
	UFUNCTION(BlueprintCallable, Category = "AI|Clearinghouse")
	void RestoreTurnResources(AActor* CurrentTurnActor);

	// 이번 턴에 연산 대상이 될 유효 타겟 목록 반환
	const TArray<TWeakObjectPtr<AActor>>& GetCachedTargets() const
	{
		return CachedTargets;
	}

protected:
	// 이번 턴에 연산 및 판단 주체가 되는 주인공 액터.
	UPROPERTY(Transient)
	TObjectPtr<AActor> ActiveTurnActor;

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


	// 타겟 당 ActionScore 선가 결과 캐시 (GetBestActionScoreTarget 연산 중복
	// 방지)
	TMap<AActor*, FPBTargetScore> CachedActionScoreMap;
};
