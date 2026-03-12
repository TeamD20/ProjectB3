// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PBCombatTypes.h"
#include "PBCombatManagerSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatStateChangedSignature, EPBCombatState);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnActiveTurnChangedSignature, AActor*, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoundChangedSignature, int32);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatantIncapacitatedSignature, AActor*);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnReactionTriggeredSignature, const FPBReactionContext&);

/**
 * 턴 관리의 핵심 서브시스템.
 * 이니셔티브 굴림, 턴 순서 정렬, 공유 턴 그룹, 상태 머신 전이, 라운드 진행을 담당한다.
 */
UCLASS()
class PROJECTB3_API UPBCombatManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/*~ UWorldSubsystem Interface ~*/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/*~ 전투 시작/종료 ~*/

	// 전투 시작. 참가자 배열을 받아 이니셔티브 굴림 → 턴 진행 시작
	void StartCombat(const TArray<AActor*>& Combatants);

	// 전투 종료. 모든 참가자에게 OnCombatEnd 통지, 상태 정리
	void EndCombat();

	// 참가자 행동불능 통지 (사망/마비/기절)
	void NotifyCombatantIncapacitated(AActor* Combatant);

	/*~ 턴 흐름 제어 ~*/

	// 현재 턴 종료 → 다음 턴으로 진행
	void EndCurrentTurn();

	// 현재 활성 전투 참가자 반환
	AActor* GetCurrentCombatant() const;

	// 현재 공유 턴 그룹의 모든 참가자 반환
	TArray<AActor*> GetCurrentSharedTurnGroup() const;

	// 공유 턴 그룹 내 다른 캐릭터로 전환
	void SwitchToGroupMember(AActor* TargetMember);

	/*~ 반응 행동 ~*/

	// 반응 트리거 발생 시 호출
	void TriggerReaction(const FPBReactionContext& Context);

	// 반응 결과 처리
	void ResolveReaction(bool bUsedReaction);

	/*~ 상태 조회 ~*/

	// 현재 전투 상태
	EPBCombatState GetCombatState() const { return CombatState; }

	// 현재 라운드 번호
	int32 GetCurrentRound() const { return CurrentRound; }

	// 전투 중 여부
	bool IsInCombat() const;

	// 이니셔티브 순서 배열 반환
	const TArray<FPBInitiativeEntry>& GetInitiativeOrder() const { return InitiativeOrder; }

	// 현재 턴 인덱스 반환
	int32 GetCurrentTurnIndex() const { return CurrentTurnIndex; }

protected:
	/*~ 이니셔티브 ~*/

	// 전체 참가자 이니셔티브 굴림 수행
	void RollInitiative();

	// D20 굴림 (이점/불이점 고려)
	int32 RollD20(bool bAdvantage, bool bDisadvantage) const;

	// 이니셔티브 정렬 (Total > DexMod > RawRoll 순 타이브레이크)
	void SortInitiativeOrder();

	/*~ 공유 턴 그룹 ~*/

	// 이니셔티브 순서상 연속인 같은 진영 캐릭터를 그룹화
	void CalculateSharedTurnGroups();

	/*~ 턴 진행 ~*/

	// 상태 전이 및 이벤트 발화
	void SetCombatState(EPBCombatState NewState);

	// 다음 턴으로 이동
	void AdvanceToNextTurn();

	// 현재 엔트리에 대해 턴 시작 처리 (행동불능이면 턴 스킵)
	void BeginTurnForCurrentEntry();

	// 라운드 종료 처리
	void HandleRoundEnd();

	// 전투 종료 조건 확인
	bool CheckCombatEndCondition() const;

	// 행동불능 처리
	void HandleIncapacitated(AActor* Combatant);
	
	// 조회
	int32 GetGroupMemberTurnIndex(int32 InGroupMemberIndex) const;
public:
	/*~ 이벤트 ~*/
	FOnCombatStateChangedSignature OnCombatStateChanged;
	FOnActiveTurnChangedSignature OnActiveTurnChanged;
	FOnRoundChangedSignature OnRoundChanged;
	FOnCombatantIncapacitatedSignature OnCombatantIncapacitated;
	FOnReactionTriggeredSignature OnReactionTriggered;
	
private:
	// 등록된 전투 참가자 목록
	TArray<TWeakObjectPtr<AActor>> RegisteredCombatants;

	// 이니셔티브 정렬된 턴 순서
	TArray<FPBInitiativeEntry> InitiativeOrder;

	// 현재 턴 인덱스
	int32 CurrentTurnIndex = 0;

	// 현재 라운드 번호
	int32 CurrentRound = 0;

	// 전투 상태
	EPBCombatState CombatState = EPBCombatState::OutOfCombat;

	// 공유 턴 그룹 내 현재 활성 멤버 인덱스
	int32 SharedTurnActiveIndex = 0;

	// 다음 공유 턴 그룹 ID 할당용
	int32 NextSharedGroupId = 0;

	// 반응 대기 중인 컨텍스트
	FPBReactionContext PendingReactionContext;

	// 공유 턴 그룹에서 이번 그룹 턴 동안 행동을 완료한 멤버 인덱스 (그룹 배열 기준)
	TSet<int32> GroupActedMemberIndices;
};
