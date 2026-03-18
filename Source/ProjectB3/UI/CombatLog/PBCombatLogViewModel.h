// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBCombatLogTypes.h"
#include "PBCombatLogViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatLogEntryAdded, const FPBCombatLogEntry& /*Entry*/);
DECLARE_MULTICAST_DELEGATE(FOnCombatLogCleared);

/** 컴뱃 로그 항목 목록을 관리하는 글로벌 ViewModel */
UCLASS()
class PROJECTB3_API UPBCombatLogViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	/*~ UPBViewModelBase Interface ~*/
	virtual void InitializeForPlayer(ULocalPlayer* InLocalPlayer) override;

	// 새 로그 엔트리를 추가한다. Round/TurnIndex/Color는 자동으로 스탬프된다.
	void AddEntry(EPBCombatLogType InLogType, const FText& LogText);

	// 시스템 메시지(턴 시작, 전투 시작 등)를 직접 추가한다.
	void AddSystemMessage(const FText& Message);

	// 현재 라운드 번호를 갱신한다.
	void SetCurrentRound(int32 InRound);

	// 현재 턴 인덱스를 갱신한다.
	void SetCurrentTurnIndex(int32 InTurnIndex);

	// 현재 라운드 번호를 반환한다.
	int32 GetCurrentRound() const { return CurrentRound; }

	// 전체 로그를 초기화한다.
	void ClearLog();

	// 전체 로그 항목을 반환한다. (위젯 초기 바인딩 시 기존 로그 복원용)
	const TArray<FPBCombatLogEntry>& GetAllEntries() const { return LogEntries; }

	// 새 엔트리가 추가될 때 브로드캐스트
	FOnCombatLogEntryAdded OnEntryAdded;

	// 로그 전체가 초기화될 때 브로드캐스트
	FOnCombatLogCleared OnLogCleared;

private:
	// 타입별 기본 색상을 초기화한다.
	void InitTypeColorMap();

	// 타입에 해당하는 색상을 반환한다.
	FLinearColor GetColorForType(EPBCombatLogType InLogType) const;

private:
	// 로그 항목 목록 (FIFO, 최대 MaxLogEntries)
	TArray<FPBCombatLogEntry> LogEntries;

	// 최대 보관 항목 수
	static constexpr int32 MaxLogEntries = 200;

	// 타입별 기본 색상 매핑
	TMap<EPBCombatLogType, FLinearColor> TypeColorMap;

	// 현재 라운드 번호
	int32 CurrentRound = 0;

	// 현재 턴 인덱스
	int32 CurrentTurnIndex = 0;
};
