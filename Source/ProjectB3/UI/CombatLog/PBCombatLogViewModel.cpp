// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCombatLogViewModel.h"

void UPBCombatLogViewModel::InitializeForPlayer(ULocalPlayer* InLocalPlayer)
{
	Super::InitializeForPlayer(InLocalPlayer);
	InitTypeColorMap();
}

void UPBCombatLogViewModel::InitTypeColorMap()
{
	TypeColorMap.Add(EPBCombatLogType::Damage,		FLinearColor(1.0f, 0.3f, 0.3f));
	TypeColorMap.Add(EPBCombatLogType::CritDamage,	FLinearColor(1.0f, 0.6f, 0.0f));
	TypeColorMap.Add(EPBCombatLogType::Heal,		FLinearColor(0.3f, 1.0f, 0.3f));
	TypeColorMap.Add(EPBCombatLogType::Miss,		FLinearColor(0.6f, 0.6f, 0.6f));
	TypeColorMap.Add(EPBCombatLogType::SaveSuccess,	FLinearColor(0.3f, 0.8f, 1.0f));
	TypeColorMap.Add(EPBCombatLogType::SaveFailed,	FLinearColor(0.8f, 0.3f, 0.8f));
	TypeColorMap.Add(EPBCombatLogType::Status,		FLinearColor(1.0f, 0.9f, 0.3f));
	TypeColorMap.Add(EPBCombatLogType::Death,		FLinearColor(0.5f, 0.1f, 0.1f));
	TypeColorMap.Add(EPBCombatLogType::System,		FLinearColor(0.8f, 0.8f, 0.8f));
}

FLinearColor UPBCombatLogViewModel::GetColorForType(EPBCombatLogType InLogType) const
{
	const FLinearColor* Found = TypeColorMap.Find(InLogType);
	return Found ? *Found : FLinearColor::White;
}

void UPBCombatLogViewModel::AddEntry(EPBCombatLogType InLogType, const FText& LogText)
{
	FPBCombatLogEntry Entry;
	Entry.LogType = InLogType;
	Entry.LogText = LogText;
	Entry.TextColor = GetColorForType(InLogType);
	Entry.Round = CurrentRound;
	Entry.TurnIndex = CurrentTurnIndex;

	// 최대 항목 수 초과 시 가장 오래된 항목 제거
	if (LogEntries.Num() >= MaxLogEntries)
	{
		LogEntries.RemoveAt(0);
	}

	LogEntries.Add(Entry);
	OnEntryAdded.Broadcast(Entry);
}

void UPBCombatLogViewModel::AddSystemMessage(const FText& Message)
{
	AddEntry(EPBCombatLogType::System, Message);
}

void UPBCombatLogViewModel::SetCurrentRound(int32 InRound)
{
	CurrentRound = InRound;
}

void UPBCombatLogViewModel::SetCurrentTurnIndex(int32 InTurnIndex)
{
	CurrentTurnIndex = InTurnIndex;
}

void UPBCombatLogViewModel::ClearLog()
{
	LogEntries.Empty();
	CurrentRound = 0;
	CurrentTurnIndex = 0;
	OnLogCleared.Broadcast();
}
