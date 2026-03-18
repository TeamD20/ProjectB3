// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBFloatingTextActor.h"
#include "PBFloatingTextWidget.h"
#include "Components/WidgetComponent.h"
#include "ProjectB3/AbilitySystem/Payload/PBFloatingTextPayload.h"

APBFloatingTextActor::APBFloatingTextActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	WidgetComp->SetupAttachment(SceneRoot);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawAtDesiredSize(true);
}

void APBFloatingTextActor::InitWithPayload(UPBFloatingTextPayload* Payload, TSubclassOf<UPBFloatingTextWidget> WidgetClass)
{
	CachedPayload = Payload;
	CachedWidgetClass = WidgetClass;
	bInitialized = true;
}

void APBFloatingTextActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bInitialized || !IsValid(CachedPayload) || !CachedWidgetClass)
	{
		Destroy();
		return;
	}

	// 위젯 클래스 설정 및 생성
	WidgetComp->SetWidgetClass(CachedWidgetClass);
	WidgetComp->InitWidget();

	UPBFloatingTextWidget* FloatingWidget = Cast<UPBFloatingTextWidget>(WidgetComp->GetWidget());
	if (!IsValid(FloatingWidget))
	{
		Destroy();
		return;
	}

	// 페이로드 바인딩 및 애니메이션 재생
	FloatingWidget->SetPayload(CachedPayload);
	FloatingWidget->OnFloatingTextAnimationFinished.BindUObject(this, &APBFloatingTextActor::OnWidgetAnimationFinished);
	FloatingWidget->PlayShowAnimation();
}

void APBFloatingTextActor::OnWidgetAnimationFinished()
{
	Destroy();
}
