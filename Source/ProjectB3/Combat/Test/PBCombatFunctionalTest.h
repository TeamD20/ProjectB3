// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "PBCombatFunctionalTest.generated.h"

class APBTestCombatCharacter;
class UPBCombatManagerSubsystem;

/**
 * 전투 턴 관리 시스템 Functional Test 기반 클래스.
 * 더미 캐릭터 스폰/정리 및 공용 헬퍼를 제공한다.
 */
UCLASS()
class PROJECTB3_API APBCombatFunctionalTestBase : public AFunctionalTest
{
	GENERATED_BODY()

public:
	APBCombatFunctionalTestBase();

protected:
	virtual void PrepareTest() override;
	virtual void CleanUp() override;

	// 더미 아군 캐릭터 스폰
	APBTestCombatCharacter* SpawnAlly(int32 InitiativeModifier = 0, const FString& Name = TEXT(""));

	// 더미 적 캐릭터 스폰
	APBTestCombatCharacter* SpawnEnemy(int32 InitiativeModifier = 0, const FString& Name = TEXT(""));

	// CombatManager 획득
	UPBCombatManagerSubsystem* GetCombatManager() const;

	// 스폰된 캐릭터를 AActor 배열로 반환
	TArray<AActor*> GetAllCombatantsAsActors() const;

	// 스폰된 캐릭터 목록
	UPROPERTY()
	TArray<TObjectPtr<APBTestCombatCharacter>> SpawnedCharacters;

private:
	int32 AllyCounter = 0;
	int32 EnemyCounter = 0;
};

/**
 * 전투 시작/종료 기본 흐름 테스트
 * - 전투 시작 시 상태 전이 확인
 * - 이니셔티브 굴림 및 정렬 확인
 * - 참가자 OnCombatBegin/End 호출 확인
 * - 전투 종료 후 OutOfCombat 복귀 확인
 */
UCLASS()
class PROJECTB3_API APBTest_CombatStartEnd : public APBCombatFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 턴 진행 및 라운드 순환 테스트
 * - EndCurrentTurn으로 턴 순서대로 진행 확인
 * - 마지막 턴 후 라운드 증가 확인
 * - 라운드 시작 시 OnRoundBegin 호출 확인
 */
UCLASS()
class PROJECTB3_API APBTest_TurnAdvanceAndRound : public APBCombatFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 이니셔티브 정렬 및 타이브레이크 테스트
 * - InitiativeTotal 내림차순 정렬 확인
 * - 동점 시 DexModifier 타이브레이크 확인
 */
UCLASS()
class PROJECTB3_API APBTest_InitiativeSorting : public APBCombatFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 공유 턴 그룹 테스트
 * - 같은 진영 연속 캐릭터가 같은 그룹 ID를 받는지 확인
 * - 다른 진영에 의해 분리된 같은 진영이 개별 턴인지 확인
 * - SwitchToGroupMember로 그룹 내 전환 확인
 */
UCLASS()
class PROJECTB3_API APBTest_SharedTurnGroup : public APBCombatFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 행동불능 처리 테스트
 * - 행동불능 캐릭터 턴 스킵 확인
 * - 현재 턴 소유자 행동불능 시 즉시 턴 종료 확인
 * - 한 진영 전멸 시 전투 종료 확인
 */
UCLASS()
class PROJECTB3_API APBTest_Incapacitation : public APBCombatFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 반응 행동 흐름 테스트
 * - TriggerReaction → WaitingForReaction 전이 확인
 * - ResolveReaction(true) 시 OnActionInterrupted 호출 + TurnInProgress 복귀 확인
 * - ResolveReaction(false) 시 바로 TurnInProgress 복귀 확인
 */
UCLASS()
class PROJECTB3_API APBTest_ReactionFlow : public APBCombatFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 델리게이트 이벤트 발화 테스트
 * - OnCombatStateChanged, OnActiveTurnChanged, OnRoundChanged 등
 *   각 상태 전이 시 올바른 이벤트가 발화되는지 확인
 */
UCLASS()
class PROJECTB3_API APBTest_DelegateEvents : public APBCombatFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};
