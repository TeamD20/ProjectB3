// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "GameFramework/GameStateBase.h"
#include "PBGameplayGameState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPartyMemeberListReadySignature, const TArray<AActor*>& /**InPartyMembers*/)
DECLARE_MULTICAST_DELEGATE(FOnCombatStartedSiganture);
// 게임 스테이트 기반 클래스. 게임 전반의 공유 상태 데이터 관리.
UCLASS()
class PROJECTB3_API APBGameplayGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	void NotifyPartyMemberListReady(const TArray<AActor*>& InPartyMembers);
	void NotifyCombatStarted();

public:
	FOnPartyMemeberListReadySignature OnPartyMemberListReady;
	FOnCombatStartedSiganture OnCombatStarted;
};
