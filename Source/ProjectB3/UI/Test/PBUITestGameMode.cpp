// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBUITestGameMode.h"
#include "Blueprint/UserWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberListViewmodel.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnOrderViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnPortraitViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnOrderInfoWidget.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnIndicatorWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/RandomShuffle.h"

void APBUITestGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("APBUITestGameMode::BeginPlay Started"));

	// 1. 더미 파티원 스폰
	SpawnDummyPartyMember();

	UE_LOG(LogTemp, Warning, TEXT("Spawned Members: %d"), SpawnPartyMembers.Num());

	// 1-5. 더미 몬스터 스폰 (최대 12마리)
	SpawnDummyMonsters();

	// 2. 데이터 셋업 (이름, HP 등)
	SetupDummyCharacterData();

	// 3. 뷰모델 라이브러리를 통해 초기화 및 데이터 등록
	InitializePartyViewModel();
	InitializeTurnViewModel();

	// 4. UI 생성 및 화면 출력
	CreatePartyUI();
	CreateTurnUI(); // 턴 전용 위젯 별도 생성
}

void APBUITestGameMode::SpawnDummyPartyMember()
{
	if (!DummyPartyMemberClass) return;

	for (int32 i = 0; i < MaxSpawnCount; ++i)
	{
		// 충돌 설정을 무시하고 강제로 스폰하도록 설정
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 임의의 위치에 스폰
		FVector SpawnLocation(i * 200.f, 0.f, 200.f); 
		AActor* NewMember = GetWorld()->SpawnActor<AActor>(DummyPartyMemberClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		
		if (NewMember)
		{
			SpawnPartyMembers.Add(NewMember);
		}
	}
}

void APBUITestGameMode::SpawnDummyMonsters()
{
	if (!DummyMonsterClass) return;

	for (int32 i = 0; i < MaxMonsterSpawnCount; ++i)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 몬스터는 파티원과 반대쪽(-Y축 방향)에 스폰
		FVector SpawnLocation(i * 150.f, -500.f, 200.f); 
		AActor* NewMonster = GetWorld()->SpawnActor<AActor>(DummyMonsterClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
		
		if (NewMonster)
		{
			SpawnMonsters.Add(NewMonster);
		}
	}
}

void APBUITestGameMode::SetupDummyCharacterData()
{
	for (int32 i = 0; i < SpawnPartyMembers.Num(); ++i)
	{
		// 뷰모델 라이브러리를 통해 액터 전용 뷰모델 생성/가져오기
		UPBPartyMemberViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), SpawnPartyMembers[i]);
		
		VM->OnPartyMemberSelected.AddUObject(this, &ThisClass::SimulateCharacterChange);
		
		if (VM)
		{
			// 클래스 풀 설정 (임시 4직업)
			TArray<FText> DummyClasses = {
				FText::FromString(TEXT("Warrior")),
				FText::FromString(TEXT("Ranger")),
				FText::FromString(TEXT("Mage")),
				FText::FromString(TEXT("Cleric"))
			};
			
			// 이름 풀에서 가져오거나 기본 이름 설정
			FText Name = DummyNamesPool.IsValidIndex(i) ? DummyNamesPool[i] : FText::Format(NSLOCTEXT("Test", "DefaultName", "Member_{0}"), i);
			VM->SetCharacterName(Name);
			
			// 직업 배정 (순서대로, 4명이 넘을 경우 사이클)
			FText CharClass = DummyClasses[i % DummyClasses.Num()];
			VM->SetCharacterClass(CharClass);
			
			// 기본 레벨 및 HP 설정
			VM->SetLevel(FMath::RandRange(1, 5));
			VM->SetHP(FMath::RandRange(30, 55), 60);
			
			// 초상화 설정
			if (DummyPortraitPool.IsValidIndex(i))
			{
				VM->SetPortrait(DummyPortraitPool[i]);
			}
			
			// 초기 턴 설정 (첫 번째 멤버)
			VM->SetIsSelectedCharacter(i == 0);
			
			VM->OnPartyMemberSelected.AddWeakLambda(this,	[this,VM](AActor* SelectedActor)
			{
				if (VM)
				{
					UE_LOG(LogTemp,Warning,TEXT("%s Selected"),*VM->GetCharacterName().ToString());
				}
			});
		}
	}
}

void APBUITestGameMode::InitializePartyViewModel()
{
	// 전역 리스트 뷰모델 가져오기
	UPBPartyMemberListViewModel* ListVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBPartyMemberListViewModel>(GetWorld()->GetFirstLocalPlayerFromController());
	
	if (ListVM)
	{
		// 스폰된 액터들을 리스트에 주입 (이 과정에서 개별 VM들이 내부적으로 생성/연결됨)
		ListVM->SetPartyMembers(SpawnPartyMembers);
	}
}

void APBUITestGameMode::CreatePartyUI()
{
	if (!PartyUIWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("PartyUIWidgetClass is NOT SET in GameMode!"));
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		CreatedPartyUIWidget = CreateWidget<UUserWidget>(PC, PartyUIWidgetClass);
		if (CreatedPartyUIWidget)
		{
			CreatedPartyUIWidget->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("Party UI Widget Added to Viewport"));
		}
	}
}

void APBUITestGameMode::CreateTurnUI()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC) return;

	if (TurnInfoHUDClass)
	{
		CreatedTurnInfoWidget = CreateWidget<UUserWidget>(PC, TurnInfoHUDClass);
		if (CreatedTurnInfoWidget)
		{
			if (UPBTurnOrderInfoWidget* InfoWidget = Cast<UPBTurnOrderInfoWidget>(CreatedTurnInfoWidget))
			{
				if (UPBTurnOrderViewModel* TurnVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBTurnOrderViewModel>(GetWorld()->GetFirstLocalPlayerFromController()))
				{
					InfoWidget->SetupViewModel(TurnVM);
				}
			}
			CreatedTurnInfoWidget->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("Turn Info HUD Widget Added to Viewport"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TurnInfoHUDClass is NOT SET in GameMode!"));
	}

	if (TurnIndicatorHUDClass)
	{
		CreatedTurnIndicatorWidget = CreateWidget<UUserWidget>(PC, TurnIndicatorHUDClass);
		if (CreatedTurnIndicatorWidget)
		{
			if (UPBTurnIndicatorWidget* IndicatorWidget = Cast<UPBTurnIndicatorWidget>(CreatedTurnIndicatorWidget))
			{
				if (UPBTurnOrderViewModel* TurnVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBTurnOrderViewModel>(GetWorld()->GetFirstLocalPlayerFromController()))
				{
					IndicatorWidget->SetupViewModel(TurnVM);
				}
			}
			CreatedTurnIndicatorWidget->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("Turn Indicator HUD Widget Added to Viewport"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TurnIndicatorHUDClass is NOT SET in GameMode!"));
	}
}

void APBUITestGameMode::SimulateCharacterChange(AActor* TargetActor)
{
	if (SpawnPartyMembers.Num() == 0) return;

	// 기존 턴 종료
	if (SpawnPartyMembers.IsValidIndex(CurrentTurnIndex))
	{
		if (UPBPartyMemberViewModel* OldVM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), SpawnPartyMembers[CurrentTurnIndex]))
		{
			OldVM->SetIsSelectedCharacter(false);
		}
	}

	// 입력된 타겟 액터가 존재하면 해당 액터의 인덱스를 찾아 지정하고, 없다면 순차 이동
	if (TargetActor)
	{
		int32 FoundIndex = SpawnPartyMembers.Find(TargetActor);
		if (FoundIndex != INDEX_NONE)
		{
			CurrentTurnIndex = FoundIndex;
		}
	}
	else
	{
		// 다음 인덱스로 이동 (기존 콘솔 커맨드용)
		CurrentTurnIndex = (CurrentTurnIndex + 1) % SpawnPartyMembers.Num();
	}

	// 새로운 턴 시작
	if (SpawnPartyMembers.IsValidIndex(CurrentTurnIndex))
	{
		if (UPBPartyMemberViewModel* NewVM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), SpawnPartyMembers[CurrentTurnIndex]))
		{
			NewVM->SetIsSelectedCharacter(true);
		}
	}
}

void APBUITestGameMode::SimulateSkillCast()
{
	// TODO: SkillHUD 구현 후 다시 작업 예정
}

void APBUITestGameMode::InitializeTurnViewModel()
{
	UPBTurnOrderViewModel* TurnVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBTurnOrderViewModel>(GetWorld()->GetFirstLocalPlayerFromController());
	
	if (TurnVM)
	{
		TArray<FPBTurnOrderEntry> TurnEntries;
        
		// 1. 파티원 (최대 4명) 등록
		for (int32 i = 0; i < SpawnPartyMembers.Num(); ++i)
		{
			FPBTurnOrderEntry Entry;
			Entry.DisplayName = DummyNamesPool.IsValidIndex(i) ? DummyNamesPool[i] : FText::Format(NSLOCTEXT("Test", "DefaultName", "Member_{0}"), i);
			Entry.Portrait = DummyPortraitPool.IsValidIndex(i) ? DummyPortraitPool[i] : nullptr;
			Entry.bIsAlly = true; // 플레이어블 파티원
			Entry.TargetActor = SpawnPartyMembers[i];

			TurnEntries.Add(Entry);
		}

		// 2. 몬스터 (최대 12마리) 등록
		for (int32 i = 0; i < SpawnMonsters.Num(); ++i)
		{
			FPBTurnOrderEntry Entry;
			// 몬스터는 임시로 "Monster_0", "Monster_1"... 등의 이름 부여
			Entry.DisplayName = FText::Format(NSLOCTEXT("Test", "MonsterName", "Monster_{0}"), i);
			Entry.Portrait = DummyMonsterPortraitPool.IsValidIndex(i) ? DummyMonsterPortraitPool[i] : nullptr;
			Entry.bIsAlly = false; // 적군
			Entry.TargetActor = SpawnMonsters[i];

			TurnEntries.Add(Entry);
		}

		// 3. 파티원과 몬스터가 무작위로 섞이도록 배열 셔플 (랜덤 섞기)
		Algo::RandomShuffle(TurnEntries);

		TurnVM->SetTurnOrder(TurnEntries);
		// Initialize the first turn
		TurnVM->AdvanceTurn();
	}
}

void APBUITestGameMode::SimulateTurnAdvance()
{
	if (UPBTurnOrderViewModel* TurnVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBTurnOrderViewModel>(GetWorld()->GetFirstLocalPlayerFromController()))
	{
		TurnVM->AdvanceTurn();
		UE_LOG(LogTemp, Warning, TEXT("SimulateTurnAdvance Called"));
	}
}

// 명시적인 이름이 필요한 경우를 위해 남겨두되, 현재는 BeginPlay에서 순차 실행
void APBUITestGameMode::SetPartyMembers() {}
void APBUITestGameMode::BindPartyDataToUI() {}
