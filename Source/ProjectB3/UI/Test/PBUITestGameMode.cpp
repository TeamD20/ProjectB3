// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBUITestGameMode.h"
#include "Blueprint/UserWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberListViewmodel.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewmodel.h"
#include "Kismet/GameplayStatics.h"

void APBUITestGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("APBUITestGameMode::BeginPlay Started"));

	// 1. 더미 파티원 스폰
	SpawnDummyPartyMember();

	UE_LOG(LogTemp, Warning, TEXT("Spawned Members: %d"), SpawnPartyMembers.Num());

	// 2. 데이터 셋업 (이름, HP 등)
	SetupDummyCharacterData();

	// 3. 뷰모델 라이브러리를 통해 초기화 및 데이터 등록
	InitializePartyViewModel();

	// 4. UI 생성 및 화면 출력
	CreatePartyUI();
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

void APBUITestGameMode::SetupDummyCharacterData()
{
	for (int32 i = 0; i < SpawnPartyMembers.Num(); ++i)
	{
		// 뷰모델 라이브러리를 통해 액터 전용 뷰모델 생성/가져오기
		UPBPartyMemberViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), SpawnPartyMembers[i]);
		
		if (VM)
		{
			// 이름 풀에서 가져오거나 기본 이름 설정
			FText Name = DummyNamesPool.IsValidIndex(i) ? DummyNamesPool[i] : FText::Format(NSLOCTEXT("Test", "DefaultName", "Member_{0}"), i);
			VM->SetCharacterName(Name);
			
			// 기본 레벨 및 HP 설정
			VM->SetLevel(FMath::RandRange(1, 5));
			VM->SetHP(FMath::RandRange(30, 55), 60);
			
			// 초상화 설정
			if (DummyPortraitPool.IsValidIndex(i))
			{
				VM->SetPortrait(DummyPortraitPool[i]);
			}
			
			// 초기 턴 설정 (첫 번째 멤버)
			VM->SetIsMyTurn(i == 0);
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

void APBUITestGameMode::SimulateTurnChange()
{
	if (SpawnPartyMembers.Num() == 0) return;

	// 기존 턴 종료
	if (SpawnPartyMembers.IsValidIndex(CurrentTurnIndex))
	{
		if (UPBPartyMemberViewModel* OldVM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), SpawnPartyMembers[CurrentTurnIndex]))
		{
			OldVM->SetIsMyTurn(false);
		}
	}

	// 다음 인덱스로 이동
	CurrentTurnIndex = (CurrentTurnIndex + 1) % SpawnPartyMembers.Num();

	// 새로운 턴 시작
	if (SpawnPartyMembers.IsValidIndex(CurrentTurnIndex))
	{
		if (UPBPartyMemberViewModel* NewVM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), SpawnPartyMembers[CurrentTurnIndex]))
		{
			NewVM->SetIsMyTurn(true);
		}
	}
}

// 명시적인 이름이 필요한 경우를 위해 남겨두되, 현재는 BeginPlay에서 순차 실행
void APBUITestGameMode::SetPartyMembers() {}
void APBUITestGameMode::BindPartyDataToUI() {}
