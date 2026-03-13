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
	void CreateMainHUD();

public:
	// Exec - 콘솔창(~)에 'SimulateTurnChange' 입력 시 바로 호출되어 실행 가능
	UFUNCTION(BlueprintCallable, Exec, Category = "Test|Simulation") 
	void SimulateCharacterChange(AActor* TargetActor = nullptr);

	// Exec - 콘솔창(~)에 'SimulateSkillCast' 입력 시 실행 (단축키 할당 목적)
	UFUNCTION(BlueprintCallable, Exec, Category = "Test|Simulation") 
	void SimulateSkillCast();
	
	// Exec - 콘솔창(~)에 'SimulateDamage' 입력 시 현재 선택된 캐릭터에게 데미지 인가
	UFUNCTION(BlueprintCallable, Exec, Category = "Test|Simulation")
	void SimulateDamage(int32 DamageAmount = 10);
	
	// Turn HUD 초기화 용도
	void InitializeTurnViewModel();

	// SkillBar HUD 더미 데이터 초기화 용도
	void InitializeSkillBarViewModel();

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

	// 통합된 메인 HUD 위젯 클래스 (GameplayHUD 용 역할)
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	TSubclassOf<UUserWidget> MainHUDClass;

	// [Step 1] 신규 메인 액션바 HUD 클래스
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	TSubclassOf<UUserWidget> MainActionBarHUDClass;

	// 스폰할 더미 몬스터 액터의 클래스
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	TSubclassOf<AActor> DummyMonsterClass;

	// 최대 스폰
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	int32 MaxSpawnCount = 4;
	
	// 최대 몬스터 스폰 마리수
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	int32 MaxMonsterSpawnCount = 12;

	// [Step 2] 스킬바 카테고리별 슬롯 수 (빈 칸 포함 공간 확보용)
	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	int32 PrimarySlotCount = 12;

	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	int32 SecondarySlotCount = 12;

	UPROPERTY(EditAnywhere, Category = "Test|Settings")
	int32 SpellSlotCount = 12;
	
	// 랜덤으로 부여할 이름들 모음
	UPROPERTY(EditAnywhere, Category = "Test|Data")
	TArray<FText> DummyNamesPool;
	
	// 랜덤으로 부여할 프로필 이미지들 모음
	UPROPERTY(EditAnywhere, Category = "Test|Data")
	TArray<TSoftObjectPtr<UTexture2D>> DummyPortraitPool;

	// 랜덤으로 부여할 몬스터 프로필 이미지들 모음
	UPROPERTY(EditAnywhere, Category = "Test|Data")
	TArray<TSoftObjectPtr<UTexture2D>> DummyMonsterPortraitPool;

	// 랜덤으로 부여할 스킬 슬롯 아이콘 모음 (SkillBar 테스트용)
	UPROPERTY(EditAnywhere, Category = "Test|Data")
	TArray<TSoftObjectPtr<UTexture2D>> DummySkillIconPool;

private:
	/* ============================================================ */
	/* 런타임 캐싱 및 상태 저장 변수 */
	/* ============================================================ */

	// 생성된 파티원들을 보관
	UPROPERTY(VisibleAnywhere, Category = "Test|Runtime")
	TArray<TObjectPtr<AActor>> SpawnPartyMembers;

	// 화면에 띄워진 통합 메인 HUD 위젯
	UPROPERTY()
	TObjectPtr<UUserWidget> CreatedMainHUDWidget;

	// 생성된 메인 액션바 HUD 위젯
	UPROPERTY()
	TObjectPtr<UUserWidget> CreatedMainActionBarWidget;

	// 생성된 몬스터들을 보관
	UPROPERTY(VisibleAnywhere, Category = "Test|Runtime")
	TArray<TObjectPtr<AActor>> SpawnMonsters;

	// 턴 변경 시뮬레이션을 위한 현재 턴 인덱스
	int32 CurrentTurnIndex = 0;
};
