// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
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
protected:
	// 게임 시작 시 자동으로 Push할 위젯 클래스 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TArray<TSubclassOf<UPBWidgetBase>> HudWidgetClasses;
};
