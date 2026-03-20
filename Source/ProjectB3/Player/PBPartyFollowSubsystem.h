// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "PBPartyFollowSubsystem.generated.h"

class APBCharacterBase;
class APBPartyAIController;
class UEnvQuery;
enum class EPBCombatState : uint8;

/** 팔로워 1명의 추적 상태 */
USTRUCT()
struct FPBFollowerState
{
	GENERATED_BODY()

	// 팔로워 캐릭터
	UPROPERTY()
	TObjectPtr<APBCharacterBase> Character = nullptr;

	// 팔로워 파티 AIC
	UPROPERTY()
	TObjectPtr<APBPartyAIController> AIC = nullptr;

	// 개별 추적 제외 플래그
	bool bUnlinked = false;

	// 연속 이동 실패 횟수
	int32 ConsecutiveFailCount = 0;
};

/** 서브시스템이 관리하는 파티 전체 Phase */
UENUM()
enum class EPBPartyFollowPhase : uint8
{
	// 대기
	Idle,
	// 리더 뒤 Trail Point 추적 중
	Trailing,
	// 리더 정지 후 Scatter 위치로 이동 중
	Scatter,
	// 전투 중 — 추적 비활성
	CombatLocked,
};

/**
 * 파티 팔로워 이동을 총괄하는 월드 서브시스템.
 * Trail Queue 관리, Phase 판정, EQS 기반 Scatter 위치 계산을 담당한다.
 */
UCLASS()
class PROJECTB3_API UPBPartyFollowSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/*~ USubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/*~ 외부 인터페이스 (PlayerController 호출) ~*/
	// FreeMovement 이동 명령 시 호출 — Trailing Phase 진입 및 타이머 시작
	void NotifyLeaderMoveStarted(APBCharacterBase* InLeader);

	// 리더 정지 감지 시 호출 — Scatter Phase 진입 및 EQS 쿼리 시작
	void NotifyLeaderMoveStopped();

	// 팔로워 개별 추적 제외/복귀
	void SetFollowerUnlinked(APBCharacterBase* Follower, bool bUnlink);

	// 전투 진입/종료 통보
	void SetCombatLocked(bool bLocked);

	// APBPartyAIController 이동 완료 콜백 (AIC가 완료 시 호출)
	void OnFollowerMoveCompleted(APBPartyAIController* AIC, bool bSuccess);

	// 현재 Phase 반환
	EPBPartyFollowPhase GetCurrentPhase() const { return CurrentPhase; }

private:
	// PlayerState에서 팔로워 목록을 읽어 Followers 캐시 갱신
	void RebuildFollowerCache();

	// Trail Point 기록 및 Idle 팔로워 이동 명령 (타이머 콜백, TrailRecordInterval 주기)
	void OnTrailUpdateTimer();

	// Scatter EQS 쿼리 실행 (비동기, 1회)
	void RunScatterEQS();

	// EQS 쿼리 완료 콜백
	void OnScatterEQSFinished(TSharedPtr<FEnvQueryResult> Result);

	// Scatter 실패 시 EQS 없이 NavMesh 원형 후보로 특정 팔로워 재배치
	void RedispatchScatterForFollower(int32 FollowerIndex);

	// 팔로워 i에게 현 시점의 Trail Point 전달 (유효성 검증 포함)
	void DispatchTrailMove(int32 FollowerIndex);

	// 팔로워 i의 Trail Point 계산: Ring Buffer에서 (i+1)*TrailSpacing 오프셋 조회
	bool GetTrailPointForFollower(int32 FollowerIndex, FVector& OutPoint) const;

	// Ring Buffer에 리더 현재 위치 Push (MinTrailDistance 초과 시에만)
	void RecordTrailPoint(const FVector& LeaderLocation);

	// NavMesh 경로 유효성 사전 검증 (레이어 3)
	bool IsPointReachable(const FVector& From, const FVector& To) const;

	// 후보 위치가 Scatter 반경 및 파티원 간 간격 조건을 만족하는지 검증
	bool IsValidScatterCandidate(const FVector& Candidate, const TArray<FVector>& OccupiedPositions, float MinSpacing) const;

	// 단일 팔로워용 Scatter 위치 선택 (InnerRadius 근접 우선, 동점 시 자기 위치 최근접)
	bool TryPickScatterPointForFollower(const FPBFollowerState& FollowerState, const TArray<FVector>& Candidates, const TSet<int32>& UsedCandidateIndices, const TArray<FVector>& OccupiedPositions, int32& OutCandidateIndex) const;

	// Phase 전환
	void SetPhase(EPBPartyFollowPhase NewPhase);

	// 전투 상태 변경 이벤트 핸들러
	void OnCombatStateChanged(EPBCombatState NewState);

private:
	/*~ 파티 상태 ~*/
	// 현재 리더 캐릭터 (PlayerController 빙의 대상)
	UPROPERTY()
	TObjectPtr<APBCharacterBase> Leader;

	// 팔로워 상태 캐시 (PlayerState에서 갱신)
	UPROPERTY()
	TArray<FPBFollowerState> Followers;

	// 현재 Follow Phase
	EPBPartyFollowPhase CurrentPhase = EPBPartyFollowPhase::Idle;

	/*~ Trail Queue (Ring Buffer) ~*/
	// 리더 이동 궤적 기록 배열
	TArray<FVector> TrailQueue;

	// 링 버퍼 최대 크기
	static constexpr int32 TrailQueueMaxSize = 128;

	// 다음 기록 위치 인덱스 (Head)
	int32 TrailHead = 0;

	// 현재 기록된 항목 수
	int32 TrailCount = 0;

	// 마지막으로 기록한 위치
	FVector LastRecordedLocation = FVector::ZeroVector;

	/*~ 타이머 ~*/
	// Trail 기록 + 팔로워 이동 명령 주기 타이머
	FTimerHandle TrailUpdateTimerHandle;

	// 진행 중인 EQS 쿼리 ID (중복 방지)
	int32 PendingEQSQueryID = INDEX_NONE;

	
	UPROPERTY()
	TObjectPtr<UEnvQuery> ScatterEQSQuery;
};
