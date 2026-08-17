// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (파티 멤버 목록 ViewModel 연결)
// Author: 강리한 (파티 멤버 툴팁 표시값 보강)

#include "PBPartyMemberListViewModel.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "PBPartyMemberViewModel.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "GameFramework/PlayerController.h"

void UPBPartyMemberListViewModel::SetPartyMembers(const TArray<AActor*>& InPartyMembers)
{
	APBGameplayPlayerState* GameplayPlayerState = nullptr;
	if (APlayerController* OwningPlayerController = GetOwningPlayerController())
	{
		GameplayPlayerState = OwningPlayerController->GetPlayerState<APBGameplayPlayerState>();
	}

	if (IsValid(GameplayPlayerState))
	{
		const TArray<AActor*> ExistingMembers = GameplayPlayerState->GetPartyMembers();
		for (AActor* ExistingMember : ExistingMembers)
		{
			if (!InPartyMembers.Contains(ExistingMember))
			{
				GameplayPlayerState->RemovePartyMember(ExistingMember);
			}
		}
	}

	MemberViewModels.Empty();
	
	
	for (AActor* Member : InPartyMembers)
	{
		if (IsValid(GameplayPlayerState))
		{
			GameplayPlayerState->AddPartyMember(Member);
		}

		UPBPartyMemberViewModel* MemberViewModel = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetLocalPlayer(), Member);
		
		MemberViewModels.Add(MemberViewModel);
	}
	
	OnPartyListChanged.Broadcast();
}
