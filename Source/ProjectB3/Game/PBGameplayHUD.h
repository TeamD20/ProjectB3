// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ProjectB3/Combat/PBCombatTypes.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "ProjectB3/UI/Combat/PBActionIndicatorTypes.h"
#include "PBGameplayHUD.generated.h"

class UPBWidgetBase;

// 게임플레이 HUD.
UCLASS()
class PROJECTB3_API APBGameplayHUD : public AHUD
{
	GENERATED_BODY()

protected:
	/*~ AHUD Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 지정 시간 뒤에 HUD 위젯 초기화를 예약한다.
	void ScheduleInitializeHUDWidgets(float DelaySeconds);

private:
	// HUD 위젯들을 UIManager에 Push한다.
	void InitializeHUDWidgets();
	
	void BindGameStateEvents();
	void UnbindGameStateEvents();

	// 이벤트 핸들러: VM 연동
	void HandlePartyMemberListReady(const TArray<AActor*>& InPartyMembers);
	void HandleSelectedPartyMemberChanged(AActor* SelectedMember);
	void HandleCombatStarted();
	void HandleActiveTurnChanged(AActor* Combatant, int32 TurnIndex);

	// 전투 상태 표시용 위젯 스폰
	void HandleCombatStateChangedForText(EPBCombatState NewState);
	
	// 스킬 이름 표시용 위젯 스폰
	void HandleSkillActivated(AActor* Caster, const FText& SkillName, EPBAbilityType AbilityType);

protected:
	// 게임 시작 시 자동으로 Push할 위젯 클래스 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TArray<TSubclassOf<UPBWidgetBase>> HudWidgetClasses;
	
public:
	// 위젯 클래스 설정 (BP에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Combat")
	TSubclassOf<class UPBCombatStateTextWidget> CombatStateTextWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Combat")
	TSubclassOf<class UPBSkillNameFloatingWidget> SkillNameFloatingWidgetClass;

private:
	// 전투 상태 인디케이터 갱신 헬퍼
	void UpdateCombatIndicator(EPBActionIndicatorType Type, const FText& Text);
	void ClearCombatIndicator();

	// 전투 인디케이터 자동 해제 타이머
	FTimerHandle CombatIndicatorTimerHandle;

	// HUD 초기화 예약 타이머
	FTimerHandle HUDInitializeTimerHandle;

	// HUD 위젯 초기화 완료 여부
	bool bHUDWidgetsInitialized = false;
};
