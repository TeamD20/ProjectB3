// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "PBPartyMemberListViewModel.h"
#include "PBPartyMemberViewModel.h"

void UPBPartyMemberListViewModel::SetPartyMembers(const TArray<AActor*>& InPartyMembers)
{
	MemberViewModels.Empty();
	
	
	for (AActor* Member : InPartyMembers)
	{
		UPBPartyMemberViewModel* MemberViewModel = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBPartyMemberViewModel>(GetLocalPlayer(), Member);
		
		MemberViewModels.Add(MemberViewModel);
	}
	
	OnPartyListChanged.Broadcast();
}
