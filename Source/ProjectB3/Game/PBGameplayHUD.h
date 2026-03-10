// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PBGameplayHUD.generated.h"

class UPBWidgetBase;

// 게임플레이 HUD. BeginPlay 시 HudWidgetClasses 목록을 UIManagerSubsystem을 통해 Push한다.
UCLASS()
class PROJECTB3_API APBGameplayHUD : public AHUD
{
	GENERATED_BODY()

protected:
	/*~ AHUD Interface ~*/
	virtual void BeginPlay() override;

private:
	// HUD 위젯들을 UIManager에 Push한다.
	void InitializeHUDWidgets();

protected:
	// 게임 시작 시 자동으로 Push할 위젯 클래스 목록
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TArray<TSubclassOf<UPBWidgetBase>> HudWidgetClasses;
};
