// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "PBCombatTypes.h"
#include "IPBCombatParticipant.generated.h"

UINTERFACE(MinimalAPI)
class UPBCombatParticipant : public UInterface
{
	GENERATED_BODY()
};

/**
 * 전투에 참여하는 모든 캐릭터가 구현해야 하는 인터페이스.
 * APBCharacterBase가 기본 구현을 제공한다.
 */
class PROJECTB3_API IPBCombatParticipant
{
	GENERATED_BODY()

public:
	/*~ 이니셔티브 ~*/

	// 이니셔티브 수정치 반환 (DEX 수정치 + 보너스)
	virtual int32 GetInitiativeModifier() const = 0;

	// 이니셔티브 이점 보유 여부
	virtual bool HasInitiativeAdvantage() const = 0;

	/*~ 턴 라이프사이클 ~*/

	// 전투 시작 시 호출
	virtual void OnCombatBegin() = 0;

	// 전투 종료 시 호출
	virtual void OnCombatEnd() = 0;

	// 라운드 시작 시 호출 (Reaction 리필)
	virtual void OnRoundBegin() = 0;

	// 턴 시작 시 호출 (Action/BonusAction/Movement 리셋)
	virtual void OnTurnBegin() = 0;

	// 턴 종료 시 호출
	virtual void OnTurnEnd() = 0;

	/*~ 반응 행동 ~*/

	// 반응 행동 가능 여부 (Reaction 자원 잔여 + 무력화 아님)
	virtual bool CanReact() const = 0;

	// 반응 기회 통지. 플레이어: 선택 UI 표시, AI: 즉시 판단
	virtual void OnReactionOpportunity(const FPBReactionContext& Context) = 0;

	// 반응 행동에 의해 진행 중인 하나의 행동이 중단될 때 호출 (턴은 종료되지 않음)
	virtual void OnActionInterrupted() = 0;

	/*~ 상태 조회 ~*/

	// 무력화 여부
	virtual bool IsIncapacitated() const = 0;

	// 진영 태그
	virtual FGameplayTag GetFactionTag() const = 0;

	// 기본 이동 속도 (cm 단위)
	virtual float GetBaseMovementSpeed() const = 0;

	// 표시 이름
	virtual FText GetCombatDisplayName() const = 0;

	// 초상화
	virtual TSoftObjectPtr<UTexture2D> GetCombatPortrait() const = 0;
};
