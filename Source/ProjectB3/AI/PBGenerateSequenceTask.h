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

	// EQS 결과로 좌표를 교체할 시퀀스 행동 인덱스
	int32 AttackMoveActionIndex = INDEX_NONE;
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

	/*~ Fallback 헬퍼 ~*/

	// Fallback 이동 후 잔여 AP로 단일 행동(Attack/Heal) 탐색.
	// FallbackPos에서의 사거리 기준으로 후보를 검색하고,
	// 최고 점수 행동을 GeneratedSequence에 추가한다.
	void TryAppendActionAfterFallback(
		const FVector& FallbackPos, float RemainingAP, float RemainingBA);

	// 현재 위치가 이미 전술적으로 유리하면 Fallback 이동 불필요 판정.
	// 사거리 내 적이 있고 + 잔여 이동력이 적으면 true 반환.
	bool ShouldSkipFallback(float RemainingMP) const;
};
