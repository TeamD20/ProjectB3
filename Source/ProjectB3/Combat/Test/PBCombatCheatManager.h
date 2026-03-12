// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "PBCombatCheatManager.generated.h"

class APBTestCombatCharacter;
class UPBCombatManagerSubsystem;

/**
 * 전투 시스템 테스트용 CheatManager.
 * 콘솔 명령(Exec)으로 전투 흐름의 각 단계를 검증한다.
 *
 * 사용법 (콘솔 ~):
 *   Combat_Start [적수]				  — 더미 캐릭터 스폰 + 전투 시작
 *   Combat_EndTurn                   — 현재 턴 종료
 *   Combat_End                       — 전투 강제 종료
 *   Combat_Kill                      — 첫 번째 생존 적 행동불능
 *   Combat_Reaction                  — 반응 트리거 테스트
 *   Combat_ResolveReaction [0/1]     — 반응 해결 (0=거부, 1=사용)
 *   Combat_SwitchMember              — 공유 턴 그룹 내 전환
 *   Combat_Status [0/1]              — 전투 상태 HUD (1=표시, 0=제거)
 *   Combat_Help                      — 명령어 목록 출력
 */
UCLASS()
class PROJECTB3_API UPBCombatCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// 전투 시작. 더미 캐릭터 스폰 후 전투 개시
	UFUNCTION(Exec)
	void Combat_Start(int32 NumEnemies = 2);

	// 현재 턴 종료
	UFUNCTION(Exec)
	void Combat_EndTurn();

	// 전투 강제 종료
	UFUNCTION(Exec)
	void Combat_End();

	// 첫 번째 생존 적 캐릭터 행동불능 처리
	UFUNCTION(Exec)
	void Combat_Kill();

	// 반응 트리거 테스트
	UFUNCTION(Exec)
	void Combat_Reaction();

	// 반응 해결 (0=거부, 1=사용)
	UFUNCTION(Exec)
	void Combat_ResolveReaction(int32 bUse = 1);

	// 공유 턴 그룹 내 다음 멤버로 전환
	UFUNCTION(Exec)
	void Combat_SwitchMember();

	// 전투 상태 HUD (1=표시, 0=제거)
	UFUNCTION(Exec)
	void Combat_Status(int32 bShow = 1);

	// 명령어 목록 출력
	UFUNCTION(Exec)
	void Combat_Help();

private:
	// 화면 Status HUD 갱신
	void RefreshStatusHUD();

	// 화면 Status HUD 제거
	void ClearStatusHUD();
	// CombatManager 서브시스템 획득
	UPBCombatManagerSubsystem* GetCombatManager() const;

	// 테스트 캐릭터 스폰
	void SpawnTestEnemies(int32 NumEnemies);

	// 테스트 캐릭터 정리
	void CleanupTestCharacters();

	// 스폰된 테스트 캐릭터들
	UPROPERTY()
	TArray<TObjectPtr<APBTestCombatCharacter>> SpawnedEnemies;

	// Status HUD 표시 여부
	bool bStatusHUDVisible = false;
};
