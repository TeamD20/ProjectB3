// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBUITestGameMode.h"

#include <ProjectB3/UI/Common/PBCombatStatsViewModel.h>

#include "Blueprint/UserWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberListViewmodel.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnOrderViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnPortraitViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnOrderInfoWidget.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnIndicatorWidget.h"
#include "Algo/RandomShuffle.h"
#include "ProjectB3/UI/SkillBar/PBSkillBarViewModel.h"

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
	InitializeSkillBarViewModel();

	// 4. UI 생성 및 화면 출력
	CreateMainHUD();
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
			
			// [새로 추가된 기능] 툴팁 UI 검증을 위한 더미 상태(Status) 및 대응(Reaction) 데이터 주입
			// 상태 효과 3개 - 아이콘은 DummySkillIconPool에서 순환 사용
			TArray<FPBPartyTooltipRowData> DummyStatuses;

			FPBPartyTooltipRowData Status1;
			Status1.Text = FText::FromString(TEXT("실바너스의 축복"));
			Status1.Icon = DummySkillIconPool.IsValidIndex(0) ? DummySkillIconPool[0] : TSoftObjectPtr<UTexture2D>();
			DummyStatuses.Add(Status1);

			FPBPartyTooltipRowData Status2;
			Status2.Text = FText::FromString(TEXT("가속도"));
			Status2.Icon = DummySkillIconPool.IsValidIndex(1) ? DummySkillIconPool[1] : TSoftObjectPtr<UTexture2D>();
			DummyStatuses.Add(Status2);

			FPBPartyTooltipRowData Status3;
			Status3.Text = FText::FromString(TEXT("위협당함"));
			Status3.Icon = DummySkillIconPool.IsValidIndex(2) ? DummySkillIconPool[2] : TSoftObjectPtr<UTexture2D>();
			DummyStatuses.Add(Status3);

			// 대응 기술 - DummyResponseIconPool을 사용하고, 스킬바에 등록된 대응 슬롯 이름과 동일하게 표시
			TArray<FPBPartyTooltipRowData> DummyReactions;
			for (int32 r = 0; r < ResponseSlotCount; ++r)
			{
				FPBPartyTooltipRowData ReactionRow;
				ReactionRow.Text = FText::FromString(FString::Printf(TEXT("대응 스킬 %d"), r + 1));
				ReactionRow.Icon = DummyResponseIconPool.IsValidIndex(r) ? DummyResponseIconPool[r] : TSoftObjectPtr<UTexture2D>();
				DummyReactions.Add(ReactionRow);
			}

			VM->SetStatusEffects(DummyStatuses);
			VM->SetReactions(DummyReactions);
			
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

void APBUITestGameMode::CreateMainHUD()
{
	if (!MainHUDClass)
	{
		UE_LOG(LogTemp, Error, TEXT("MainHUDClass is NOT SET in GameMode!"));
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		CreatedMainHUDWidget = CreateWidget<UUserWidget>(PC, MainHUDClass);
		if (CreatedMainHUDWidget)
		{
			CreatedMainHUDWidget->AddToViewport();
			UE_LOG(LogTemp, Warning, TEXT("Main HUD Widget Added to Viewport"));
		}

		// [Step 1] 메인 액션바 HUD 생성 및 추가
		if (MainActionBarHUDClass)
		{
			CreatedMainActionBarWidget = CreateWidget<UUserWidget>(PC, MainActionBarHUDClass);
			if (CreatedMainActionBarWidget)
			{
				CreatedMainActionBarWidget->AddToViewport();
				UE_LOG(LogTemp, Warning, TEXT("Main Action Bar HUD Added to Viewport"));
			}
		}
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

			// 초기 체력 전달
			if (UPBPartyMemberViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), SpawnPartyMembers[i]))
			{
				Entry.InitialHealthPercent = VM->GetHealthPercent();
			}

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

void APBUITestGameMode::InitializeSkillBarViewModel()
{
	UPBSkillBarViewModel* SkillBarVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBSkillBarViewModel>(GetWorld()->GetFirstLocalPlayerFromController());
	
	if (SkillBarVM)
	{
		// 임의로 각 탭에 넣을 더미 데이터 생성 (빈 슬롯 포함)
		auto CreateDummySlots = [this](int32 DataCount, int32 TotalCapacity, const FString& Prefix, const TArray<TSoftObjectPtr<UTexture2D>>& IconPool, const FText& InSkillType) -> TArray<FPBSkillSlotData>
		{
			TArray<FPBSkillSlotData> DummySlots;
			for (int32 i = 0; i < TotalCapacity; ++i)
			{
				FPBSkillSlotData SlotData;
				
				// 실 데이터가 존재하는 인덱스라면 정보 기입
				if (i < DataCount)
				{
					SlotData.DisplayName = FText::FromString(FString::Printf(TEXT("%s Skill %d"), *Prefix, i + 1));
					SlotData.SkillType = InSkillType;
					
					if (i == 0)
					{
						// 첫 번째: 활성화 상태 (포커스 테두리)
						SlotData.bCanActivate = true;
						SlotData.CooldownRemaining = 0;
						SlotData.bIsActive = true;

						// [툴팁 테스트용 더미 데이터 주입]
						SlotData.Description = FText::FromString(TEXT("강력한 마법의 힘을 응축하여 전방에 발사합니다."));
						SlotData.DamageDesc = FText::FromString(TEXT("1~10 피해"));
						SlotData.DiceRollDesc = FText::FromString(TEXT("1d10 역장"));
						SlotData.RollType = FText::FromString(TEXT("명중 굴림"));
						SlotData.RollTypeEnum = EPBDiceRollType::HitRoll;
						SlotData.ActionRange = FText::FromString(TEXT("18m"));
						// 주사위 아이콘/색상은 더미용 임시 텍스처나 기본값을 사용 (아이콘이 없으면 Blueprint 더미가 출력됨)
					}
					else if (i == 1)
					{
						// 두 번째: 비활성화 상태 (DisabledOverlay)
						SlotData.bCanActivate = false;
						SlotData.CooldownRemaining = 0;
						SlotData.bIsActive = false;

						// [툴팁 테스트용 더미 데이터 주입]
						SlotData.Description = FText::FromString(TEXT("아군의 상처를 초자연적인 힘으로 즉시 치유합니다."));
						SlotData.DamageDesc = FText::FromString(TEXT("2~8 회복"));
						SlotData.DiceRollDesc = FText::FromString(TEXT("2d4 생명"));
						SlotData.RollType = FText::FromString(TEXT("자동 적중 (회복)"));
						SlotData.RollTypeEnum = EPBDiceRollType::None;
						SlotData.ActionRange = FText::FromString(TEXT("접촉"));
					}
					else if (i == 2)
					{
						// 세 번째: 쿨타임 상태 (CooldownOverlay & Text)
						SlotData.bCanActivate = true;
						SlotData.CooldownRemaining = 12.0f;
						SlotData.bIsActive = false;
					}
					else
					{
						// 나머지: 기본 상태
						SlotData.bCanActivate = true;
						SlotData.CooldownRemaining = 0;
						SlotData.bIsActive = false;
					}
					
					if (IconPool.Num() > 0)
					{
						SlotData.Icon = IconPool[i % IconPool.Num()];
					}
				}
				else
				{
					// 빈 슬롯 (공간 확보용)
					SlotData.DisplayName = FText::GetEmpty();
					SlotData.bCanActivate = false;
					SlotData.bIsActive = false;
					SlotData.CooldownRemaining = 0;
				}
				
				DummySlots.Add(SlotData);
			}
			return DummySlots;
		};

		// GameMode 프로퍼티에 설정된 수만큼 슬롯 생성 (실제 데이터는 일부만 채움)
		SkillBarVM->PrimaryActions = CreateDummySlots(10, PrimarySlotCount, TEXT("Primary"), DummySkillIconPool, FText::FromString(TEXT("주 행동")));
		SkillBarVM->SecondaryActions = CreateDummySlots(8, SecondarySlotCount, TEXT("Secondary"), DummySkillIconPool, FText::FromString(TEXT("보조 행동")));
		SkillBarVM->SpellActions = CreateDummySlots(5, SpellSlotCount, TEXT("Spell"), DummySkillIconPool, FText::FromString(TEXT("주문")));
		
		// [ Step 5 테스트 ] 대응 스킬 유동적 조절 (Response)
		SkillBarVM->ResponseActions = CreateDummySlots(ResponseSlotCount, ResponseSlotCount, TEXT("Reaction"), DummyResponseIconPool, FText::FromString(TEXT("대응")));

		auto CreateDummyEquipment = [this](int32 Count, bool bIsUtility) -> TArray<FPBEquipmentSlotData>
		{
			TArray<FPBEquipmentSlotData> DummySlots;
			for (int32 i = 0; i < Count; ++i)
			{
				FPBEquipmentSlotData SlotData;
				SlotData.bIsAvailable = true;
				
				if (bIsUtility)
				{
					SlotData.Quantity = FMath::RandRange(2, 10);
				}
				
				if (DummySkillIconPool.Num() > 0)
				{
					// 임시로 스킬 아이콘 풀을 장비/아이템에도 사용합니다
					SlotData.Icon = DummySkillIconPool[(i + (bIsUtility ? 3 : 0)) % DummySkillIconPool.Num()];
				}
				DummySlots.Add(SlotData);
			}
			return DummySlots;
		};

		// 주무기 슬롯(2개) 및 유틸리티 슬롯(4개) 임시 더미 데이터 주입
		SkillBarVM->WeaponSlots = CreateDummyEquipment(2, false);
		SkillBarVM->UtilitySlots = CreateDummyEquipment(4, true);
		
		// 스킬 및 아이템 UI 갱신 이벤트 트리거
		SkillBarVM->OnSlotsChanged.Broadcast();
		
		UE_LOG(LogTemp, Warning, TEXT("SkillBar ViewModel Initialized with Dummy Data."));
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

void APBUITestGameMode::SimulateDamage(int32 DamageAmount)
{
	// 현재 포커스 맞춰진(선택된) 캐릭터(CurrentTurnIndex 위치)에게 데미지 적용
	if (SpawnPartyMembers.IsValidIndex(CurrentTurnIndex))
	{
		AActor* TargetActor = SpawnPartyMembers[CurrentTurnIndex];
		if (UPBPartyMemberViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), TargetActor))
		{
			// 현재 체력에서 감산 (최소 0 유지)
			int32 NewHP = FMath::Max(0, VM->GetCurrentHP() - DamageAmount);
			VM->SetHP(NewHP, VM->GetMaxHP());
			
			UE_LOG(LogTemp, Warning, TEXT("[SimulateDamage] %s took %d damage. Current HP: %d / %d"), 
				*VM->GetCharacterName().ToString(), DamageAmount, NewHP, VM->GetMaxHP());

			// [임시 테스트 로직] 턴 오더 HUD의 초상화 뷰모델에도 데미지(체력 비율) 정보를 동기화
			if (UPBTurnOrderViewModel* TurnVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBTurnOrderViewModel>(GetWorld()->GetFirstLocalPlayerFromController()))
			{
				for (UPBTurnPortraitViewModel* PortraitVM : TurnVM->GetPortraitViewModels())
				{
					// 더미 캐릭터의 이름이 동일한 초상화 뷰모델을 찾아 퍼센트 전달
					if (PortraitVM && PortraitVM->GetDisplayName().EqualTo(VM->GetCharacterName()))
					{
						PortraitVM->SetHealthPercent(VM->GetHealthPercent());
						break;
					}
				}
			}
		}
	}
}

void APBUITestGameMode::SimulateResourceUse(int32 ResourceTypeEnumIndex)
{
	EPBResourceType TargetType = static_cast<EPBResourceType>(ResourceTypeEnumIndex);
	
	// 1. 현재 포커스 맞춰진(선택된) 캐릭터(CurrentTurnIndex 위치)의 CombatStatsVM을 찾습니다.
	if (SpawnPartyMembers.IsValidIndex(CurrentTurnIndex))
	{
		AActor* TargetActor = SpawnPartyMembers[CurrentTurnIndex];
		if (UPBCombatStatsViewModel* CombatVM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBCombatStatsViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), TargetActor))
		{
			FPBResourceState State;
			if (CombatVM->GetResourceState(TargetType, State))
			{
				if (State.CurrentValue > 0)
				{
					CombatVM->SetResourceState(TargetType, State.CurrentValue - 1, State.MaxValue);
					UE_LOG(LogTemp, Warning, TEXT("[SimulateResourceUse] Used Resource %d. Current: %d / %d"), 
						ResourceTypeEnumIndex, State.CurrentValue - 1, State.MaxValue);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[SimulateResourceUse] No Resource left! Type: %d"), ResourceTypeEnumIndex);
				}
			}
			else
			{
				// 처음 사용하는 자원일 경우, 임시로 최대 3을 주고 2로 감소시켜 생성
				CombatVM->SetResourceState(TargetType, 2, 3);
				UE_LOG(LogTemp, Warning, TEXT("[SimulateResourceUse] Initialized Dummy Resource %d. Current: 2 / 3"), ResourceTypeEnumIndex);
			}
		}
	}
}

void APBUITestGameMode::SimulateResourceRestore()
{
	if (SpawnPartyMembers.IsValidIndex(CurrentTurnIndex))
	{
		AActor* TargetActor = SpawnPartyMembers[CurrentTurnIndex];
		if (UPBCombatStatsViewModel* CombatVM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBCombatStatsViewModel>(GetWorld()->GetFirstLocalPlayerFromController(), TargetActor))
		{
			// 주요 3가지 자원 모두 최대치로 회복 시뮬레이션
			CombatVM->SetResourceState(EPBResourceType::Action, 3, 3);
			CombatVM->SetResourceState(EPBResourceType::BonusAction, 1, 1);
			CombatVM->SetResourceState(EPBResourceType::SpellSlot, 4, 4);

			UE_LOG(LogTemp, Warning, TEXT("[SimulateResourceRestore] All Resources Restored for Current Turn Character!"));
		}
	}
}
