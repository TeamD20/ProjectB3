// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayHUD.h"

#include "PBGameplayGameState.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/PBUIManagerSubsystem.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberListViewModel.h"
#include "ProjectB3/UI/PartyMemeber/PBPartyMemberViewmodel.h"
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
			VM->SetHP(1,1); // TODO: ASC 바인드
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
			// TODO: TurnOrderEntry에서 HealthPercent제거?
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
		CombatManager->OnCombatStateChanged.AddWeakLambda(this,[VM](EPBCombatState CombatState)
		{
			if (CombatState == EPBCombatState::CombatEnding)
			{
				TArray<FPBTurnOrderEntry> EmptyTurn;
				VM->SetTurnOrder(EmptyTurn);
				// TODO: EmptyTurn 전달 대신 Visibilty제어.
			}
		});
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
	
	// 전투 중인 경우
	UPBCombatManagerSubsystem* CombatManager = GetWorld()->GetSubsystem<UPBCombatManagerSubsystem>();
	if (!IsValid(CombatManager))
	{
		return;
	}
	
	if (CombatManager->IsInCombat())
	{
		TArray<AActor*> TurnGroup = CombatManager->GetCurrentSharedTurnGroup();
		if (TurnGroup.Contains(SelectedMember))
		{
			CombatManager->SwitchToGroupMember(SelectedMember);
		}
	}
}
