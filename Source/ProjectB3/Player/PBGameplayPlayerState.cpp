// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBGameplayPlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

void APBGameplayPlayerState::AddPartyMember(AActor* PartyMember)
{
	if (!IsValid(PartyMember))
	{
		return;
	}

	for (const TWeakObjectPtr<AActor>& ExistingMember : PartyMembers)
	{
		if (ExistingMember.Get() == PartyMember)
		{
			return;
		}
	}

	PartyMembers.Add(PartyMember);

	if (!IsValid(SelectedPartyMember.Get()))
	{
		SelectPartyMember(PartyMember);
	}
}

void APBGameplayPlayerState::RemovePartyMember(AActor* PartyMember)
{
	if (!IsValid(PartyMember))
	{
		return;
	}

	PartyMembers.RemoveAll(
		[PartyMember](const TWeakObjectPtr<AActor>& ExistingMember)
		{
			return !IsValid(ExistingMember.Get()) || ExistingMember.Get() == PartyMember;
		});

	if (SelectedPartyMember.Get() == PartyMember)
	{
		SelectedPartyMember.Reset();
		OnSelectedPartyMemberChanged.Broadcast(nullptr);
	}
}

void APBGameplayPlayerState::SelectPartyMember(AActor* PartyMember)
{
	if (PartyMember != nullptr && !IsValid(PartyMember))
	{
		return;
	}

	if (SelectedPartyMember.Get() == PartyMember)
	{
		return;
	}

	SelectedPartyMember = PartyMember;
	OnSelectedPartyMemberChanged.Broadcast(SelectedPartyMember.Get());
}

TArray<AActor*> APBGameplayPlayerState::GetPartyMembers() const
{
	TArray<AActor*> Result;
	Result.Reserve(PartyMembers.Num());

	for (const TWeakObjectPtr<AActor>& PartyMember : PartyMembers)
	{
		if (IsValid(PartyMember.Get()))
		{
			Result.Add(PartyMember.Get());
		}
	}

	return Result;
}

AActor* APBGameplayPlayerState::GetSelectedPartyMember() const
{
	return SelectedPartyMember.Get();
}

bool APBGameplayPlayerState::RequestAbilityActivation(FGameplayAbilitySpecHandle AbilityHandle) const
{
	if (!AbilityHandle.IsValid())
	{
		return false;
	}

	AActor* SelectedMember = SelectedPartyMember.Get();
	if (!IsValid(SelectedMember))
	{
		return false;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(SelectedMember);
	if (AbilitySystemInterface == nullptr)
	{
		return false;
	}

	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent))
	{
		return false;
	}

	return AbilitySystemComponent->TryActivateAbility(AbilityHandle);
}
