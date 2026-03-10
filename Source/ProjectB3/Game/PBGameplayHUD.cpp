// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayHUD.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/UI/PBUIManagerSubsystem.h"

void APBGameplayHUD::BeginPlay()
{
	Super::BeginPlay();

	// PlayerController 초기화 및 기타 월드 액터들의 초기화가 완료된 이후 위젯을 Push한다.
	GetWorldTimerManager().SetTimerForNextTick(this, &APBGameplayHUD::InitializeHUDWidgets);
}

void APBGameplayHUD::InitializeHUDWidgets()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return;
	}

	UPBUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UPBUIManagerSubsystem>();
	if (!IsValid(UIManager))
	{
		return;
	}

	for (TSubclassOf<UPBWidgetBase> WidgetClass : HudWidgetClasses)
	{
		if (!WidgetClass)
		{
			continue;
		}
		UIManager->PushUI(WidgetClass);
	}
}
