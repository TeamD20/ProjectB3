// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayGameMode.h"

#include "PBGameplayGameState.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void APBGameplayGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// 기본 파티 스폰/빙의에 성공하면 DefaultPawn 스폰 경로를 타지 않는다.
	if (SpawnDefaultPartyAndPossess(NewPlayer))
	{
		return;
	}

	// 파티 스폰 설정이 비어있거나 실패한 경우에만 기존 GameMode 시작 로직으로 폴백한다.
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

bool APBGameplayGameMode::SpawnDefaultPartyAndPossess(APlayerController* NewPlayer)
{
	// 플레이어 컨트롤러가 없거나 기본 파티 클래스가 비어 있으면 커스텀 스폰을 수행할 수 없다.
	if (!IsValid(NewPlayer) || DefaultPartyMemberClasses.Num() == 0)
	{
		return false;
	}

	// PlayerStart를 기준으로 파티를 일렬 배치한다. Start가 없으면 월드 원점에 스폰한다.
	AActor* PlayerStart = FindPlayerStart(NewPlayer);
	const FVector StartLocation = IsValid(PlayerStart)
		? PlayerStart->GetActorLocation()
		: FVector::ZeroVector;
	const FRotator StartRotation = IsValid(PlayerStart)
		? PlayerStart->GetActorRotation()
		: FRotator::ZeroRotator;

	TArray<TObjectPtr<APBCharacterBase>> NewlySpawnedPartyMembers;
	NewlySpawnedPartyMembers.Reserve(DefaultPartyMemberClasses.Num());

	FActorSpawnParameters SpawnParams;
	// 초기 테스트 단계에서는 스폰 실패를 최소화하기 위해 충돌 조정 후 강제 스폰한다.
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 ClassIndex = 0; ClassIndex < DefaultPartyMemberClasses.Num(); ++ClassIndex)
	{
		TSubclassOf<APBCharacterBase> PartyMemberClass = DefaultPartyMemberClasses[ClassIndex];
		if (!IsValid(PartyMemberClass))
		{
			// 배열에 빈 엔트리가 있어도 나머지 파티원 스폰은 계속 진행한다.
			continue;
		}

		// Y축 방향으로 간격을 두고 순차 배치한다.
		const FVector SpawnOffset(0.0f, PartySpawnSpacing * ClassIndex, 0.0f);
		const FVector SpawnLocation = StartLocation + SpawnOffset;

		APBCharacterBase* SpawnedPartyMember = GetWorld()->SpawnActor<APBCharacterBase>(
			PartyMemberClass,
			SpawnLocation,
			StartRotation,
			SpawnParams);
		if (IsValid(SpawnedPartyMember))
		{
			NewlySpawnedPartyMembers.Add(SpawnedPartyMember);
		}
	}

	if (NewlySpawnedPartyMembers.Num() == 0)
	{
		// 단 한 명도 스폰되지 않으면 상위 시작 로직으로 되돌린다.
		return false;
	}

	// PlayerState 파티 목록/선택 상태를 런타임 UI(ViewModel) 경로와 동일하게 초기화한다.
	if (APBGameplayPlayerState* PS = NewPlayer->GetPlayerState<APBGameplayPlayerState>())
	{
		for (APBCharacterBase* PartyMember : NewlySpawnedPartyMembers)
		{
			PS->AddPartyMember(PartyMember);
		}
		
		if (APBGameplayGameState* GS = GetGameState<APBGameplayGameState>())
		{
			GS->NotifyPartyMemberListReady(PS->GetPartyMembers());
		}
		
		// 첫 번째 파티원을 초기 선택 대상으로 지정한다. (내부에서 Possess 처리)
		PS->SelectPartyMember(NewlySpawnedPartyMembers[0]);
	}

	SetPlayerDefaults(NewlySpawnedPartyMembers[0]);
	
	return true;
}

void APBGameplayGameMode::InitiateCombat(const TArray<AActor*>& Combatants)
{
	UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
	if (!CombatManager)
	{
		return;
	}
	
	CombatManager->StartCombat(Combatants);
	
	if (APBGameplayGameState* GS = GetGameState<APBGameplayGameState>())
	{
		GS->NotifyCombatStarted();
	}
}

bool APBGameplayGameMode::CheckGameOver() const
{
	if (!GetWorld())
	{
		return false;
	}
	
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!IsValid(PC))
	{
		return false;
	}
	
	APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>();
	if (!IsValid(PS))
	{
		return false;
	}
	
	for (AActor* PartyMember : PS->GetPartyMembers())
	{
		if (IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(PartyMember))
		{
			// 한 명이라도 살아있으면 GameOver 실패
			if (!CPI->IsDead())
			{
				return false;
			}
		}
	}
	
	// 파티 전멸 -> GameOver
	return true;
}
