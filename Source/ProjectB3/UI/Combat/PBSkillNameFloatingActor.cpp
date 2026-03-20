// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillNameFloatingActor.h"
#include "PBSkillNameFloatingWidget.h"
#include "Components/WidgetComponent.h"

APBSkillNameFloatingActor::APBSkillNameFloatingActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	WidgetComp->SetupAttachment(SceneRoot);
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawAtDesiredSize(true);
}

void APBSkillNameFloatingActor::InitSkillName(const FText& SkillName, EPBAbilityType AbilityType, TSubclassOf<UPBSkillNameFloatingWidget> WidgetClass)
{
	CachedSkillName = SkillName;
	CachedAbilityType = AbilityType;
	CachedWidgetClass = WidgetClass;
	bInitialized = true;
}

void APBSkillNameFloatingActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bInitialized || !CachedWidgetClass)
	{
		Destroy();
		return;
	}

	WidgetComp->SetWidgetClass(CachedWidgetClass);
	WidgetComp->InitWidget();

	UPBSkillNameFloatingWidget* SkillWidget = Cast<UPBSkillNameFloatingWidget>(WidgetComp->GetWidget());
	if (!IsValid(SkillWidget))
	{
		Destroy();
		return;
	}

	SkillWidget->OnAnimFinished.BindUObject(this, &APBSkillNameFloatingActor::OnWidgetAnimationFinished);
	SkillWidget->SetSkillInfo(CachedSkillName, CachedAbilityType);
	SkillWidget->PlayShowAnimation();
}

void APBSkillNameFloatingActor::OnWidgetAnimationFinished()
{
	Destroy();
}
