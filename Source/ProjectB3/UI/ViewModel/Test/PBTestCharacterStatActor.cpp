// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTestCharacterStatActor.h"
#include "ProjectB3/UI/ViewModel/Example/PBExampleCharacterStatViewModel.h"
#include "ProjectB3/UI/ViewModel/Example/PBExampleCharacterStatWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

APBTestCharacterStatActor::APBTestCharacterStatActor()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentHP = 38;
	MaxHP = 42;
	CurrentLevel = 5;
}

void APBTestCharacterStatActor::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UPBViewModelBase* VMBase = UPBUIBlueprintLibrary::GetOrCreateActorViewModel(PC, this, UPBExampleCharacterStatViewModel::StaticClass());
		if (UPBExampleCharacterStatViewModel* CharVM = Cast<UPBExampleCharacterStatViewModel>(VMBase))
		{
			CharVM->SetCharacterName(FText::FromString(TEXT("Shadowheart")));
			CharVM->SetLevel(CurrentLevel);
			CharVM->SetHP(CurrentHP, MaxHP);
			CharVM->SetArmorClass(16);
			CharVM->SetAbilityScores(13, 13, 14, 10, 17, 12);
			CharVM->SetDesiredVisibility(true);
		}
	}
}

void APBTestCharacterStatActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void APBTestCharacterStatActor::TestTakeDamage()
{
	CurrentHP -= 5;
	if (CurrentHP < 0)
	{
		CurrentHP = 0;
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UPBViewModelBase* VMBase = UPBUIBlueprintLibrary::FindActorViewModel(PC, this, UPBExampleCharacterStatViewModel::StaticClass());
		if (UPBExampleCharacterStatViewModel* CharVM = Cast<UPBExampleCharacterStatViewModel>(VMBase))
		{
			CharVM->SetHP(CurrentHP, MaxHP);
		}
	}
}

void APBTestCharacterStatActor::TestLevelUp()
{
	CurrentLevel++;
	MaxHP += 8;
	CurrentHP += 8;

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UPBViewModelBase* VMBase = UPBUIBlueprintLibrary::FindActorViewModel(PC, this, UPBExampleCharacterStatViewModel::StaticClass());
		if (UPBExampleCharacterStatViewModel* CharVM = Cast<UPBExampleCharacterStatViewModel>(VMBase))
		{
			CharVM->SetLevel(CurrentLevel);
			CharVM->SetHP(CurrentHP, MaxHP);
		}
	}
}

void APBTestCharacterStatActor::TestOpenStatSheet()
{
	UE_LOG(LogTemp, Warning, TEXT("[PBTestCharacterStatActor] TestOpenStatSheet Triggered!"));

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	// PushUI로 캐릭터 시트 팝업 생성
	StatSheetWidget = UPBUIBlueprintLibrary::PushUI(PC, UPBExampleCharacterStatWidget::StaticClass());

	// 위젯에 이 Actor를 바인딩
	if (UPBExampleCharacterStatWidget* StatWidget = Cast<UPBExampleCharacterStatWidget>(StatSheetWidget.Get()))
	{
		StatWidget->SetTargetActor(this);
	}
}

void APBTestCharacterStatActor::TestCloseStatSheet()
{
	UE_LOG(LogTemp, Warning, TEXT("[PBTestCharacterStatActor] TestCloseStatSheet Triggered!"));

	if (!StatSheetWidget.IsValid())
	{
		return;
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UPBUIBlueprintLibrary::PopUI(PC, StatSheetWidget.Get());
	}
	StatSheetWidget.Reset();
}
