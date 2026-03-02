// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBViewModelBase.h"
#include "Engine/LocalPlayer.h"

void UPBViewModelBase::InitializeForPlayer(ULocalPlayer* InLocalPlayer)
{
	LocalPlayer = InLocalPlayer;
	TargetActor.Reset();
}

void UPBViewModelBase::InitializeForActor(AActor* InTargetActor, ULocalPlayer* InLocalPlayer)
{
	LocalPlayer = InLocalPlayer;
	TargetActor = InTargetActor;
}

void UPBViewModelBase::Deinitialize()
{
	bIsVisible = false;
	bDesiredVisibility = false;
	bVisibilityOverrideActive = false;
	bVisibilityOverrideValue = false;
	LocalPlayer.Reset();
	TargetActor.Reset();
}

void UPBViewModelBase::SetDesiredVisibility(bool bNewVisible)
{
	bDesiredVisibility = bNewVisible;
	UpdateEffectiveVisibility();
}

void UPBViewModelBase::SetVisibilityOverride(bool bOverrideVisible)
{
	bVisibilityOverrideActive = true;
	bVisibilityOverrideValue = bOverrideVisible;
	UpdateEffectiveVisibility();
}

void UPBViewModelBase::ClearVisibilityOverride()
{
	bVisibilityOverrideActive = false;
	UpdateEffectiveVisibility();
}

void UPBViewModelBase::UpdateEffectiveVisibility()
{
	const bool bNewEffective = bVisibilityOverrideActive ? bVisibilityOverrideValue : bDesiredVisibility;
	if (bIsVisible != bNewEffective)
	{
		bIsVisible = bNewEffective;
		OnVisibilityChanged.Broadcast(bIsVisible);
	}
}

APlayerController* UPBViewModelBase::GetOwningPlayerController() const
{
	if (LocalPlayer.IsValid())
	{
		return LocalPlayer->GetPlayerController(LocalPlayer->GetWorld());
	}
	return nullptr;
}
