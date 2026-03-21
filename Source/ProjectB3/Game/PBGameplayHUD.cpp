// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayHUD.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "PBGameplayGameState.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/Attributes/PBCharacterAttributeSet.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/PBUIManagerSubsystem.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberListViewModel.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewmodel.h"
#include "ProjectB3/UI/Combat/PBCombatStateTextActor.h"
#include "ProjectB3/UI/Combat/PBCombatStateTextWidget.h"
#include "ProjectB3/UI/Combat/PBSkillNameFloatingActor.h"
#include "ProjectB3/UI/Combat/PBSkillNameFloatingWidget.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"
#include "ProjectB3/UI/Combat/PBActionIndicatorViewModel.h"
#include "ProjectB3/UI/CombatLog/PBCombatLogViewModel.h"
#include "ProjectB3/UI/TurnInfoHUD/PBTurnOrderViewModel.h"

void APBGameplayHUD::BeginPlay()
{
	Super::BeginPlay();

	BindGameStateEvents();
	// PlayerController 초기화 및 기타 월드 액터들의 초기화가 완료된 이후 위젯을 Push한다.
	GetWorldTimerManager().SetTimerForNextTick(this, &APBGameplayHUD::InitializeHUDWidgets);
}

void APBGameplayHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	UnbindGameStateEvents();
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

void APBGameplayHUD::BindGameStateEvents()
{
	APBGameplayGameState* GS = GetWorld()->GetGameState<APBGameplayGameState>();
	if (!IsValid(GS))
	{
		return;
	}
	
	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return;
	}
	
	APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}
	
	// 이벤트 바인딩
	GS->OnPartyMemberListReady.AddUObject(this,&ThisClass::HandlePartyMemberListReady);
	GS->OnCombatStarted.AddUObject(this,&ThisClass::HandleCombatStarted);
	
	PS->OnSelectedPartyMemberChanged.AddUObject(this, &ThisClass::HandleSelectedPartyMemberChanged);
	
	// 현재 정보 조회하여 초기화
	HandlePartyMemberListReady(PS->GetPartyMembers());
	HandleSelectedPartyMemberChanged(PS->GetSelectedPartyMember());

	if (UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		CombatManager->OnCombatStateChanged.AddUObject(this, &ThisClass::HandleCombatStateChangedForText);
		CombatManager->OnSkillActivated.AddUObject(this, &ThisClass::HandleSkillActivated);
	}
}

void APBGameplayHUD::UnbindGameStateEvents()
{
	if (APBGameplayGameState* GS = GetWorld()->GetGameState<APBGameplayGameState>())
	{
		GS->OnPartyMemberListReady.RemoveAll(this);
		GS->OnCombatStarted.RemoveAll(this);
	}
	
	if (APlayerController* PC = GetOwningPlayerController())
	{
		if (APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>())
		{
			PS->OnSelectedPartyMemberChanged.RemoveAll(this);
		}
	}

	if (UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>())
	{
		CombatManager->OnCombatStateChanged.RemoveAll(this);
		CombatManager->OnSkillActivated.RemoveAll(this);
	}
}

void APBGameplayHUD::HandlePartyMemberListReady(const TArray<AActor*>& InPartyMembers)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return;
	}
	
	APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}
	
	if (UPBPartyMemberListViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBPartyMemberListViewModel>(PC))
	{
		VM->SetPartyMembers(InPartyMembers);
	}
	
	for (AActor* PartyMember : InPartyMembers)
	{
		UPBPartyMemberViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetOwningPlayerController(), PartyMember);
		if (!VM)
		{
			continue;
		}
		
		if (IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(PartyMember))
		{
			VM->SetPortrait(CPI->GetCombatPortrait());
			VM->SetCharacterName(CPI->GetCombatDisplayName());
			VM->SetCharacterClass(FText::FromString(TEXT("임시 직업"))); // TODO: 클래스 조회
			VM->SetLevel(0); // TODO: 레벨 조회.
			// HP는 UPBAbilitySystemUIBridge가 ASC Attribute 변경 구독으로 자동 갱신
		}
		
		VM->OnPartyMemberSelected.RemoveAll(this);
		VM->OnPartyMemberSelected.AddWeakLambda(this, [this](AActor* SelectedMember)
		{
			APlayerController* PC = GetOwningPlayerController();
			if (!IsValid(PC))
			{
				return;
			}
			
			APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>();
			if (!IsValid(PS))
			{
				return;
			}
			
			PS->SelectPartyMember(SelectedMember);
		});
	}
}

void APBGameplayHUD::HandleCombatStarted()
{
	UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
	if (!IsValid(CombatManager))
	{
		return;
	}

	TArray<FPBInitiativeEntry> InitiativeOrder = CombatManager->GetInitiativeOrder();
	TArray<FPBTurnOrderEntry> UITurnOrder;
	
	for (FPBInitiativeEntry& InitiativeEntry : InitiativeOrder)
	{
		FPBTurnOrderEntry TurnOrderEntry;
		TurnOrderEntry.TargetActor = InitiativeEntry.Combatant.Get();
		if (IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(InitiativeEntry.Combatant.Get()))
		{
			TurnOrderEntry.DisplayName = CPI->GetCombatDisplayName();
			TurnOrderEntry.Portrait = CPI->GetCombatPortrait();
			TurnOrderEntry.bIsAlly = !CPI->GetFactionTag().MatchesTagExact(PBGameplayTags::Combat_Faction_Player);
		}
		
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InitiativeEntry.Combatant.Get()))
		{
			float MaxHP = ASC->GetNumericAttribute(UPBCharacterAttributeSet::GetMaxHPAttribute());
			float HP = ASC->GetNumericAttribute(UPBCharacterAttributeSet::GetHPAttribute());
			float Percent = MaxHP == 0.f ? 0.f : HP / MaxHP;
			TurnOrderEntry.InitialHealthPercent = Percent;	
		}
		
		UITurnOrder.Add(TurnOrderEntry);
	}
	
	if (UPBTurnOrderViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBTurnOrderViewModel>(GetOwningPlayerController()))
	{
		VM->SetTurnOrder(UITurnOrder);
		VM->SetTurnIndex(0);

		CombatManager->OnActiveTurnChanged.RemoveAll(this);
		CombatManager->OnActiveTurnChanged.AddUObject(this, &ThisClass::HandleActiveTurnChanged);

		CombatManager->OnCombatStateChanged.RemoveAll(this);
		CombatManager->OnCombatStateChanged.AddWeakLambda(this,[VM, this](EPBCombatState CombatState)
		{
			if (CombatState == EPBCombatState::CombatEnding)
			{
				TArray<FPBTurnOrderEntry> EmptyTurn;
				VM->SetTurnOrder(EmptyTurn);
				// TODO: EmptyTurn 전달 대신 Visibilty제어.

				// 전투 종료 로그 메시지 (로그는 유지하여 플레이어가 결과 확인 가능)
				if (UPBCombatLogViewModel* LogVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBCombatLogViewModel>(GetOwningPlayerController()))
				{
					LogVM->AddSystemMessage(NSLOCTEXT("PBCombatLog", "CombatEnd", "[전투 종료]"));
				}
			}
		});
	}

	// 전투 시작 로그
	if (UPBCombatLogViewModel* LogVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBCombatLogViewModel>(GetOwningPlayerController()))
	{
		LogVM->ClearLog();
		LogVM->SetCurrentRound(1);
		LogVM->AddSystemMessage(NSLOCTEXT("PBCombatLog", "CombatStart", "[전투 시작]"));
	}
}

void APBGameplayHUD::HandleActiveTurnChanged(AActor* Combatant, int32 TurnIndex)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return;
	}

	// VM TurnIndex 갱신
	if (UPBTurnOrderViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBTurnOrderViewModel>(PC))
	{
		VM->SetTurnIndex(TurnIndex);
	}

	// 플레이어 턴이 되었을 때 해당 캐릭터로 자동 선택
	if (APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>())
	{
		TArray<AActor*> PartyMembers = PS->GetPartyMembers();
		if (PartyMembers.Contains(Combatant))
		{
			PS->SelectPartyMember(Combatant);
		}
	}

	// 턴 시작 로그
	if (UPBCombatLogViewModel* LogVM = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBCombatLogViewModel>(PC))
	{
		// 라운드 순환 감지: TurnIndex가 0으로 돌아오면 다음 라운드
		if (TurnIndex == 0 && LogVM->GetCurrentRound() > 0)
		{
			const int32 NextRound = LogVM->GetCurrentRound() + 1;
			LogVM->SetCurrentRound(NextRound);
			LogVM->AddSystemMessage(FText::Format(
				NSLOCTEXT("PBCombatLog", "RoundStart", "=== 라운드 {0} ==="),
				FText::AsNumber(NextRound)));
		}

		LogVM->SetCurrentTurnIndex(TurnIndex);

		FText TurnActorName = NSLOCTEXT("PBCombatLog", "UnknownCombatant", "알 수 없는 전투원");
		if (IsValid(Combatant))
		{
			if (IPBCombatParticipant* CPI = Cast<IPBCombatParticipant>(Combatant))
			{
				TurnActorName = CPI->GetCombatDisplayName();
			}
		}

		LogVM->AddSystemMessage(FText::Format(
			NSLOCTEXT("PBCombatLog", "TurnStart", "[{0}의 턴]"),
			TurnActorName));
	}
}

void APBGameplayHUD::HandleSelectedPartyMemberChanged(AActor* SelectedMember)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC))
	{
		return;
	}
	
	APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}
		
	for (AActor* PartyMember : PS->GetPartyMembers())
	{
		UPBPartyMemberViewModel* VM = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetOwningPlayerController(), PartyMember);
		if (!VM)
		{
			continue;
		}
		
		VM->SetIsSelectedCharacter(PartyMember == SelectedMember);
	}
}

void APBGameplayHUD::HandleCombatStateChangedForText(EPBCombatState NewState)
{
	if (NewState == EPBCombatState::CombatStarting || NewState == EPBCombatState::CombatEnding)
	{
		// 1. 내 화면의 HUD 인디케이터(MainHUD 하위 등)에 "전투 시작!" / "전투 종료!" 표시 (위젯 클래스 설정 무관)
		if (NewState == EPBCombatState::CombatStarting)
		{
			UpdateCombatIndicator(EPBActionIndicatorType::CombatStart, NSLOCTEXT("PBUI", "CombatStart", "전투 시작!"));
		}
		else if (NewState == EPBCombatState::CombatEnding)
		{
			UpdateCombatIndicator(EPBActionIndicatorType::CombatEnd, NSLOCTEXT("PBUI", "CombatEnd", "전투 종료!"));
		}

		// 2. 캐릭터 머리 위 전투 상태 텍스트 스폰 (위젯 클래스가 등록되어야만 동작)
		if (IsValid(CombatStateTextWidgetClass))
		{
			UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
			if (IsValid(CombatManager))
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

				// 전투 참가자 전체 위에 스폰
				for (const FPBInitiativeEntry& Entry : CombatManager->GetInitiativeOrder())
				{
					if (AActor* Combatant = Entry.Combatant.Get())
					{
						if (APBCombatStateTextActor* TextActor = GetWorld()->SpawnActor<APBCombatStateTextActor>(Combatant->GetActorLocation(), FRotator::ZeroRotator, SpawnParams))
						{
							TextActor->InitCombatState(NewState == EPBCombatState::CombatStarting, CombatStateTextWidgetClass);
						}
					}
				}
			}
		}
	}
}

void APBGameplayHUD::HandleSkillActivated(AActor* Caster, const FText& SkillName, EPBAbilityType AbilityType)
{
	if (!IsValid(SkillNameFloatingWidgetClass) || !IsValid(Caster))
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector Location = Caster->GetActorLocation();
	
	// 소유 캐릭터보다 약간 더 위에 스폰되도록 높이 조정 (데미지 팝업과 겹침 방지 등)
	Location.Z += 80.0f;

	if (APBSkillNameFloatingActor* TextActor = GetWorld()->SpawnActor<APBSkillNameFloatingActor>(Location, FRotator::ZeroRotator, SpawnParams))
	{
		TextActor->InitSkillName(SkillName, AbilityType, SkillNameFloatingWidgetClass);
	}
}

void APBGameplayHUD::UpdateCombatIndicator(EPBActionIndicatorType Type, const FText& Text)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC) || !IsValid(GetWorld()))
	{
		return;
	}

	UPBViewModelSubsystem* VMSubsystem = PC->GetLocalPlayer()->GetSubsystem<UPBViewModelSubsystem>();
	if (!IsValid(VMSubsystem))
	{
		return;
	}

	// 내 캐릭터(Pawn)의 ActionIndicator 뷰모델을 갱신
	APawn* MyPawn = PC->GetPawn();
	if (!IsValid(MyPawn))
	{
		// 폰이 아직 빙의되지 않았을 수 있으니 PlayerController를 오너로 삼는 뷰모델 시도 (보통은 폰 기준)
		// 현재 프로젝트는 컴뱃의 주체가 캐릭터(Pawn)이므로 일단 무시
		return;
	}

	UPBActionIndicatorViewModel* VM = VMSubsystem->GetOrCreateActorViewModel<UPBActionIndicatorViewModel>(MyPawn);
	if (IsValid(VM))
	{
		FPBActionIndicatorData ActionData;
		ActionData.ActionType = Type;
		ActionData.DisplayText = Text;
		ActionData.bIsActive = true;

		VM->SetAction(ActionData);

		// 기존 타이머가 작동 중이면 리셋
		GetWorld()->GetTimerManager().ClearTimer(CombatIndicatorTimerHandle);

		// 텍스트를 2.5초 간 보여준 뒤 자동으로 ClearAction() 호출
		GetWorld()->GetTimerManager().SetTimer(
			CombatIndicatorTimerHandle,
			this,
			&APBGameplayHUD::ClearCombatIndicator,
			2.5f,
			false
		);
	}
}

void APBGameplayHUD::ClearCombatIndicator()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!IsValid(PC) || !IsValid(GetWorld()))
	{
		return;
	}

	UPBViewModelSubsystem* VMSubsystem = PC->GetLocalPlayer()->GetSubsystem<UPBViewModelSubsystem>();
	if (!IsValid(VMSubsystem))
	{
		return;
	}

	APawn* MyPawn = PC->GetPawn();
	if (!IsValid(MyPawn))
	{
		return;
	}

	UPBActionIndicatorViewModel* VM = VMSubsystem->GetOrCreateActorViewModel<UPBActionIndicatorViewModel>(MyPawn);
	if (IsValid(VM))
	{
		VM->ClearAction();
	}
}
