// PBGenerateSequenceTask.h
#pragma once

#include "Blueprint/StateTreeTaskBlueprintBase.h"
#include "CoreMinimal.h"
#include "PBAITypes.h"
#include "PBGenerateSequenceTask.generated.h"

class UEnvQuery;
class UPBUtilityClearinghouse;

// Clearinghouse에 질의하여 데이터를 수집하고, 최적의 행동 조합(Sequence)을 큐
// 형태로 생성하는 블루프린트 호환 테스크.
// Phase 3: DFS 완료 후 EQS 비동기 쿼리로 Move 좌표를 최적화한다.
UCLASS(Blueprintable, meta = (DisplayName = "Generate AI Action Sequence",
	Category = "AI|Sequence"))
class PROJECTB3_API UPBGenerateSequenceTask
	: public UStateTreeTaskBlueprintBase
{
	GENERATED_BODY()

public:
	UPBGenerateSequenceTask(const FObjectInitializer& ObjectInitializer);

	/*~ 입력 (Input) 바인딩 핀 ~*/
	// StateTree에서 바인딩 받을 현재 턴을 수행 중인 AI 액터
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Input")
	TObjectPtr<AActor> SelfActor = nullptr;

	/*~ 출력 (Output) 바인딩 핀 ~*/
	// 이 테스크가 뱉어낼 최종 콤보 시퀀스 결과물 데이터 (부모 방향으로 출력 배출
	// 허용을 위해 Output 카테고리 유지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
	FPBActionSequence GeneratedSequence;

	/*~ EQS 쿼리 에셋 (에디터에서 할당) ~*/

	// 공격 위치 탐색 쿼리 (EQS_FindAttackPosition)
	// null이면 EQS 스킵, DFS 원래 좌표 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQS")
	TObjectPtr<UEnvQuery> AttackPositionQuery = nullptr;

	// 후퇴 위치 탐색 쿼리 (EQS_FindFallbackPosition)
	// null이면 EQS 스킵, CalculateFallbackPosition 결과 사용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EQS")
	TObjectPtr<UEnvQuery> FallbackPositionQuery = nullptr;

	/*~ UStateTreeTaskBlueprintBase Interface ~*/
protected:
	virtual EStateTreeRunStatus
	EnterState(FStateTreeExecutionContext& Context,
	           const FStateTreeTransitionResult& Transition) override;

	virtual EStateTreeRunStatus
	Tick(FStateTreeExecutionContext& Context, const float DeltaTime) override;

	virtual void
	ExitState(FStateTreeExecutionContext& Context,
	          const FStateTreeTransitionResult& Transition) override;

private:
	/*~ EQS 비동기 상태 ~*/

	// EQS 쿼리 완료 대기 중 여부
	bool bWaitingForEQS = false;

	// 진행 중인 EQS 쿼리 수 (0이 되면 대기 해제)
	int32 PendingEQSQueryCount = 0;

	// EQS 타임아웃 잔여 시간 (초)
	float EQSTimeoutRemaining = 0.0f;

	// EQS Attack 쿼리 직렬 실행 큐.
	// LaunchEQSQueries에서 모든 Attack-Move 인덱스를 적재하고,
	// 첫 번째만 발사. 콜백 완료 시 다음을 발사하여
	// Clearinghouse의 EQS 파라미터(TargetActor/MaxRange/IdealDist) 오염을 방지.
	TArray<int32> PendingAttackMoveIndices;

	// 현재 처리 중인 Attack-Move 인덱스 (콜백에서 사용)
	int32 CurrentAttackMoveIndex = INDEX_NONE;

	int32 FallbackMoveActionIndex = INDEX_NONE;

	// 캐싱된 Clearinghouse (EnterState에서 획득)
	UPROPERTY()
	TObjectPtr<UPBUtilityClearinghouse> CachedClearinghouse = nullptr;

	/*~ EQS 콜백 ~*/

	void OnAttackPositionQueryFinished(bool bSuccess, const FVector& Location);
	void OnFallbackPositionQueryFinished(bool bSuccess, const FVector& Location);

	// 모든 EQS 쿼리 완료 시 bIsReady = true 전환
	void CheckAllEQSComplete();

	// EQS 상태 초기화 (EnterState/ExitState 공용)
	void ResetEQSState();

	// 시퀀스 내 Move 행동에 대해 EQS 위치 최적화 쿼리를 발사한다.
	// EnterState의 여러 분기(타겟 없음, DFS 후, DFS 실패)에서 공통 호출.
	void LaunchEQSQueries();

	// PendingAttackMoveIndices 큐에서 다음 Attack-Move EQS 쿼리를 발사한다.
	// Clearinghouse EQS 파라미터 오염 방지를 위해 한 번에 하나씩 직렬 실행.
	void LaunchNextAttackQuery();

	/*~ Move 분리 헬퍼 (Phase 3) ~*/

	// DFS 결과 시퀀스에서 MovementCost > 0인 행동 앞에
	// 물리적 Move 노드를 삽입한다.
	// DFS는 "무엇을 할지"만 결정하고, 이 함수가 "이동"을 명시화한다.
	static void InjectMoveActions(FPBActionSequence& Sequence);

	/*~ Fallback 헬퍼 ~*/

	// Fallback 이동 후 잔여 AP로 단일 행동(Attack/Heal) 탐색.
	// FallbackPos에서의 사거리 기준으로 후보를 검색하고,
	// 최고 점수 행동을 GeneratedSequence에 추가한다.
	void TryAppendActionAfterFallback(
		const FVector& FallbackPos, float RemainingAP, float RemainingBA);

	// 현재 위치가 이미 전술적으로 유리하면 Fallback 이동 불필요 판정.
	// ASC 어빌리티 사거리 + 2D 거리 + LoS 확인.
	// 교전 가능한 적이 있으면 true 반환.
	bool ShouldSkipFallback(float RemainingMP) const;

	// EQS 후검증에서 Move+Attack이 무효화되었을 때,
	// 캐시된 스코어 맵에서 현재 위치 기준 대안 행동을 탐색하여
	// None 처리된 Attack 슬롯을 교체한다.
	// 대안이 없으면 None 유지 (기존 동작과 동일).
	void TryReplaceInvalidatedActions(
		int32 InvalidatedMoveIdx, int32 InvalidatedAttackIdx);
};
