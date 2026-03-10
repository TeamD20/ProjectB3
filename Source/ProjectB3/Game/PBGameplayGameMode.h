// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "GameFramework/GameModeBase.h"
#include "PBGameplayGameMode.generated.h"

class APBCharacterBase;
class APlayerController;

// 게임 모드 기반 클래스. 게임 규칙, 승패 조건, 플로우 제어 등 전반적인 게임 진행 관리.
UCLASS()
class PROJECTB3_API APBGameplayGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 플레이어 입장 시 기본 파티를 스폰하고 첫 번째 캐릭터를 빙의시킨다.
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	// 전투 개시. CombatManagerSubsystem에 위임
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void InitiateCombat(const TArray<AActor*>& Combatants);

private:
	// PlayerStart 기준으로 기본 파티를 스폰하고 첫 번째 캐릭터를 빙의한다.
	bool SpawnDefaultPartyAndPossess(APlayerController* NewPlayer);

public:
	// 플레이 시작 시 기본으로 스폰할 파티원 클래스 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Party")
	TArray<TSubclassOf<APBCharacterBase>> DefaultPartyMemberClasses;

protected:
	// 파티 캐릭터 스폰 시 캐릭터 간 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Party")
	float PartySpawnSpacing = 150.0f;

};
