// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PBUITestGameMode.generated.h"

class UUserWidget;

/**
 * UI 뷰모델 연동 테스트를 위한 게임모드입니다.
 */
UCLASS()
class PROJECTB3_API APBUITestGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	
	void InitializePartyViewModel();
	void SetPartyMembers();

protected:
	void SpawnDummyPartyMember();
	void SpawnDummyMonsters();
	void SetupDummyCharacterData();
	void BindPartyDataToUI();
	void CreatePartyUI();
	void CreateTurnUI();

public:
	// Exec - 콘솔창(~)에 'SimulateTurnChange' 입력 시 바로 호출되어 실행 가능
	UFUNCTION(BlueprintCallable, Exec, Category = "Test|Simulation") 
	void SimulateCharacterChange(AActor* TargetActor = nullptr);

	// Exec - 콘솔창(~)에 'SimulateSkillCast' 입력 시 실행 (단축키 할당 목적)
	UFUNCTION(BlueprintCallable, Exec, Category = "Test|Simulation") 
	void SimulateSkillCast();
	
	// Turn HUD 초기화 용도
	void InitializeTurnViewModel();

	// Exec - 콘솔창(~)에 'SimulateTurnAdvance' 입력 시 턴 진행 시뮬레이션
	UFUNCTION(BlueprintCallable, Exec, Category = "Test|Simulation")
	void SimulateTurnAdvance();
	
protected:
	/* ============================================================ */
	/* 에디터 설정 변수 (블루프린트에서 값과 클래스를 할당해 줍니다) */
	/* ============================================================ */

	// 스폰할 더미 파티원 액터의 클래스
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	TSubclassOf<AActor> DummyPartyMemberClass;

	// 화면에 띄울 파티 리스트 메인 UI 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	TSubclassOf<UUserWidget> PartyUIWidgetClass;

	// 화면에 띄울 상단 턴 순서 리스트 UI 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	TSubclassOf<UUserWidget> TurnInfoHUDClass;

	// 화면 중앙 턴 알림 인디케이터 UI 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	TSubclassOf<UUserWidget> TurnIndicatorHUDClass;

	// 스폰할 더미 몬스터 액터의 클래스
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	TSubclassOf<AActor> DummyMonsterClass;

	// 최대 스폰 개수
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	int32 MaxSpawnCount = 4;
	
	// 최대 몬스터 스폰 개수
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	int32 MaxMonsterSpawnCount = 12;
	
	// 랜덤으로 부여할 이름들 모음
	UPROPERTY(EditAnywhere, Category = "Test|Data")
	TArray<FText> DummyNamesPool;
	
	// 랜덤으로 부여할 프로필 이미지들 모음
	UPROPERTY(EditAnywhere, Category = "Test|Data")
	TArray<TSoftObjectPtr<UTexture2D>> DummyPortraitPool;

	// 랜덤으로 부여할 몬스터 프로필 이미지들 모음
	UPROPERTY(EditAnywhere, Category = "Test|Data")
	TArray<TSoftObjectPtr<UTexture2D>> DummyMonsterPortraitPool;

private:
	/* ============================================================ */
	/* 런타임 캐싱 및 상태 저장 변수 */
	/* ============================================================ */

	// 생성된 파티원들을 보관
	UPROPERTY(VisibleAnywhere, Category = "Test|Runtime")
	TArray<TObjectPtr<AActor>> SpawnPartyMembers;

	// 화면에 생성되어 띄워진 파티 UI 위젯 임시 보관
	UPROPERTY()
	TObjectPtr<UUserWidget> CreatedPartyUIWidget;
	
	// 화면에 생성되어 띄워진 턴 정보 UI 위젯 임시 보관
	UPROPERTY()
	TObjectPtr<UUserWidget> CreatedTurnInfoWidget;

	// 화면에 생성되어 띄워진 턴 인디케이터 UI 위젯 임시 보관
	UPROPERTY()
	TObjectPtr<UUserWidget> CreatedTurnIndicatorWidget;

	// 생성된 몬스터들을 보관
	UPROPERTY(VisibleAnywhere, Category = "Test|Runtime")
	TArray<TObjectPtr<AActor>> SpawnMonsters;

	// 턴 변경 시뮬레이션을 위한 현재 턴 인덱스
	int32 CurrentTurnIndex = 0;
};
