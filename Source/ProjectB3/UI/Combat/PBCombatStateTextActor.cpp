// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBCombatStateTextActor.h"
#include "PBCombatStateTextWidget.h"
#include "Components/WidgetComponent.h"

APBCombatStateTextActor::APBCombatStateTextActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	WidgetComp->SetupAttachment(SceneRoot);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawAtDesiredSize(true);
}

void APBCombatStateTextActor::InitCombatState(bool bIsStarting, TSubclassOf<UPBCombatStateTextWidget> WidgetClass)
{
	bCachedIsStarting = bIsStarting;
	CachedWidgetClass = WidgetClass;
	bInitialized = true;
}

void APBCombatStateTextActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bInitialized || !CachedWidgetClass)
	{
		Destroy();
		return;
	}

	// 위젯 생성
	WidgetComp->SetWidgetClass(CachedWidgetClass);
	WidgetComp->InitWidget();

	UPBCombatStateTextWidget* StateWidget = Cast<UPBCombatStateTextWidget>(WidgetComp->GetWidget());
	if (!IsValid(StateWidget))
	{
		Destroy();
		return;
	}

	// 위젯 애니메이션 종료 시 Actor 자동 제거 바인딩
	StateWidget->OnAnimFinished.BindUObject(this, &APBCombatStateTextActor::OnWidgetAnimationFinished);
	
	// 상태 문자열 및 애니메이션 트리거
	StateWidget->SetCombatState(bCachedIsStarting);
}

void APBCombatStateTextActor::OnWidgetAnimationFinished()
{
	Destroy();
}
