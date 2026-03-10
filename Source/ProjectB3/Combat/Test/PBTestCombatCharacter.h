// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "PBTestCombatCharacter.generated.h"

/**
 * 전투용 더미 캐릭터.
 * 테스트 시 IPBCombatParticipant의 동작을 검증하기 위한 최소 구현.
 */
UCLASS()
class PROJECTB3_API APBTestCombatCharacter : public APBCharacterBase
{
	GENERATED_BODY()

public:
	APBTestCombatCharacter();

	/*~ IPBCombatParticipant Interface ~*/

	// 이니셔티브 수정치 (에디터에서 설정)
	virtual int32 GetInitiativeModifier() const override;

	// 이니셔티브 이점 여부 (에디터에서 설정)
	virtual bool HasInitiativeAdvantage() const override;

	// 무력화 여부 (에디터에서 설정)
	virtual bool IsIncapacitated() const override;

	// 반응 가능 여부 (ASC 없이 직접 제어)
	virtual bool CanReact() const override;

	// 무력화 상태 설정 (테스트용)
	void SetIncapacitated(bool bNewIncapacitated);

	// 반응 가능 여부 설정 (테스트용)
	void SetCanReact(bool bNewCanReact);

	// OnTurnBegin/OnTurnActivated 호출 횟수 초기화
	void ResetTurnCallCounts();

	/*~ IPBCombatParticipant Interface ~*/

	// OnTurnBegin 호출 시 카운터 증가
	virtual void OnTurnBegin() override;

	// OnTurnActivated 호출 시 카운터 증가
	virtual void OnTurnActivated() override;

public:
	// 이니셔티브 수정치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Combat")
	int32 TestInitiativeModifier = 0;

	// 이니셔티브 이점 보유 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Combat")
	bool bTestHasAdvantage = false;

	// 무력화 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Combat")
	bool bTestIsIncapacitated = false;

	// 반응 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test|Combat")
	bool bTestCanReact = true;

	// OnTurnBegin 호출 횟수 (검증용)
	int32 TurnBeginCount = 0;

	// OnTurnActivated 호출 횟수 (검증용)
	int32 TurnActivatedCount = 0;
};
