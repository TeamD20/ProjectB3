// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (전투 기능 검증용 캐릭터)

#include "PBTestCombatCharacter.h"

APBTestCombatCharacter::APBTestCombatCharacter()
{
}

int32 APBTestCombatCharacter::GetInitiativeModifier() const
{
	return TestInitiativeModifier;
}

bool APBTestCombatCharacter::HasInitiativeAdvantage() const
{
	return bTestHasAdvantage;
}

bool APBTestCombatCharacter::IsIncapacitated() const
{
	return bTestIsIncapacitated;
}

bool APBTestCombatCharacter::IsDead() const
{
	// 테스트 더미는 무력화를 곧 전투 이탈(사망)로 간주한다.
	return bTestIsIncapacitated;
}

void APBTestCombatCharacter::SetIncapacitated(bool bNewIncapacitated)
{
	bTestIsIncapacitated = bNewIncapacitated;
}

bool APBTestCombatCharacter::CanReact() const
{
	return !bTestIsIncapacitated && bTestCanReact;
}

void APBTestCombatCharacter::SetCanReact(bool bNewCanReact)
{
	bTestCanReact = bNewCanReact;
}

void APBTestCombatCharacter::ResetTurnCallCounts()
{
	TurnBeginCount = 0;
	TurnActivatedCount = 0;
}

void APBTestCombatCharacter::OnRoundBegin()
{
	// 더미는 턴 자원(GAS 어트리뷰트)을 사용하지 않으므로 Super의 Reaction 리필을 생략.
	// (ASC에 턴 자원 어트리뷰트 셋이 미등록 상태라 SetAttributeBase에서 ensure가 발생하는 것을 방지)
}

void APBTestCombatCharacter::OnTurnBegin()
{
	// Super(Action/BonusAction/Movement 리셋)를 생략 — 위 OnRoundBegin과 동일한 이유.
	// 테스트는 턴 자원이 아닌 호출 횟수만 검증하므로 카운터만 증가.
	TurnBeginCount++;
}

void APBTestCombatCharacter::OnTurnActivated()
{
	Super::OnTurnActivated();
	TurnActivatedCount++;
}
