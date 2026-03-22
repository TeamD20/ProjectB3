// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PBCombatTypes.generated.h"

/**
 * 전투 상태 열거형. UPBCombatManagerSubsystem이 소유하고 전이시킨다.
 */
UENUM(BlueprintType)
enum class EPBCombatState : uint8
{
	OutOfCombat,
	CombatStarting,
	InitiativeRoll,
	TurnInProgress,
	WaitingForReaction,
	TurnEnding,
	RoundEnding,
	CombatEnding
};

/**
 * 턴 자원 종류. 어빌리티 비용 지정 및 UI 표시에 사용.
 */
UENUM(BlueprintType)
enum class EPBTurnResourceType : uint8
{
	Action,
	BonusAction,
	Reaction,
	Movement
};

/**
 * 반응 행동 트리거 유형.
 */
UENUM(BlueprintType)
enum class EPBReactionTrigger : uint8
{
	// 적이 근접 범위를 벗어날 때 (기회 공격)
	LeavesMeleeRange,
	// 적이 주문을 시전할 때 (주문 차단)
	CastsSpell,
	// 공격이 명중하려 할 때 (방패 등 방어 반응)
	AboutToBeHit
};

/**
 * 반응 행동 컨텍스트. 반응 기회 발생 시 판단에 필요한 정보.
 */
USTRUCT(BlueprintType)
struct FPBReactionContext
{
	GENERATED_BODY()

	// 반응 트리거 유형
	UPROPERTY(BlueprintReadOnly)
	EPBReactionTrigger Trigger = EPBReactionTrigger::LeavesMeleeRange;

	// 반응 대상 (트리거를 유발한 캐릭터)
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> TriggeringActor;

	// 반응을 수행할 수 있는 캐릭터 (반응 기회를 받은 측)
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> ReactingActor;
};

/**
 * 이니셔티브 엔트리. 이니셔티브 굴림 결과를 저장하며 턴 순서를 결정한다.
 */
USTRUCT(BlueprintType)
struct FPBInitiativeEntry
{
	GENERATED_BODY()

	// 전투 참가자 레퍼런스
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> Combatant;

	// 이니셔티브 총합 (d20 + modifier)
	UPROPERTY(BlueprintReadOnly)
	int32 InitiativeTotal = 0;

	// d20 원본 굴림값 (타이브레이크 용)
	UPROPERTY(BlueprintReadOnly)
	int32 RawRoll = 0;

	// DEX 수정치 (타이브레이크 용)
	UPROPERTY(BlueprintReadOnly)
	int32 DexModifier = 0;

	// 공유 턴 그룹 ID (-1이면 개별 턴)
	UPROPERTY(BlueprintReadOnly)
	int32 SharedTurnGroupId = -1;
};
