// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBUIBlueprintLibrary.h"
#include "PBUIManagerSubsystem.h"
#include "ViewModel/PBViewModelBase.h"
#include "ViewModel/PBViewModelSubsystem.h"

ULocalPlayer* UPBUIBlueprintLibrary::GetLocalPlayerFromController(APlayerController* OwningPlayer)
{
	if (!IsValid(OwningPlayer))
	{
		return nullptr;
	}
	return OwningPlayer->GetLocalPlayer();
}

UPBUIManagerSubsystem* UPBUIBlueprintLibrary::GetUIManager(APlayerController* OwningPlayer, EPBValidResult& Result)
{
	UPBUIManagerSubsystem* Manager = GetUIManager(OwningPlayer);
	Result = Manager ? EPBValidResult::Valid : EPBValidResult::Invalid;
	return Manager;
}

UPBUIManagerSubsystem* UPBUIBlueprintLibrary::GetUIManager(APlayerController* OwningPlayer)
{
	return GetUIManager(GetLocalPlayerFromController(OwningPlayer));
}

UPBUIManagerSubsystem* UPBUIBlueprintLibrary::GetUIManager(const ULocalPlayer* LocalPlayer)
{
	return LocalPlayer ? LocalPlayer->GetSubsystem<UPBUIManagerSubsystem>() : nullptr;
}

UPBWidgetBase* UPBUIBlueprintLibrary::PushUI(APlayerController* OwningPlayer, TSubclassOf<UPBWidgetBase> WidgetClass, EPBValidResult& Result)
{
	UPBWidgetBase* Widget = PushUI(OwningPlayer, WidgetClass);
	Result = Widget ? EPBValidResult::Valid : EPBValidResult::Invalid;
	return Widget;
}

UPBWidgetBase* UPBUIBlueprintLibrary::PushUI(APlayerController* OwningPlayer, TSubclassOf<UPBWidgetBase> WidgetClass)
{
	if (UPBUIManagerSubsystem* UIManager = GetUIManager(OwningPlayer))
	{
		return UIManager->PushUI(WidgetClass);
	}
	return nullptr;
}

void UPBUIBlueprintLibrary::PopUI(APlayerController* OwningPlayer, UPBWidgetBase* WidgetInstance)
{
	if (UPBUIManagerSubsystem* UIManager = GetUIManager(OwningPlayer))
	{
		UIManager->PopUI(WidgetInstance);
	}
}

UPBViewModelSubsystem* UPBUIBlueprintLibrary::GetViewModelSubsystem(APlayerController* OwningPlayer, EPBValidResult& Result)
{
	UPBViewModelSubsystem* VMS = GetViewModelSubsystem(OwningPlayer);
	Result = VMS ? EPBValidResult::Valid : EPBValidResult::Invalid;
	return VMS;
}

UPBViewModelSubsystem* UPBUIBlueprintLibrary::GetViewModelSubsystem(APlayerController* OwningPlayer)
{
	return GetViewModelSubsystem(GetLocalPlayerFromController(OwningPlayer));
}

UPBViewModelSubsystem* UPBUIBlueprintLibrary::GetViewModelSubsystem(const ULocalPlayer* LocalPlayer)
{
	return LocalPlayer ? LocalPlayer->GetSubsystem<UPBViewModelSubsystem>() : nullptr;
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel(APlayerController* OwningPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass, EPBValidResult& Result)
{
	UPBViewModelBase* VM = GetOrCreateGlobalViewModel(OwningPlayer, ViewModelClass);
	Result = VM ? EPBValidResult::Valid : EPBValidResult::Invalid;
	return VM;
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel(APlayerController* OwningPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	return GetOrCreateGlobalViewModel(GetLocalPlayerFromController(OwningPlayer), ViewModelClass);
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel(const ULocalPlayer* LocalPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	if (UPBViewModelSubsystem* VMS = GetViewModelSubsystem(LocalPlayer))
	{
		return VMS->GetOrCreateGlobalViewModel(ViewModelClass);
	}
	return nullptr;
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetGlobalViewModel(APlayerController* OwningPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass, EPBFoundResult& Result)
{
	UPBViewModelBase* VM = GetGlobalViewModel(OwningPlayer, ViewModelClass);
	Result = VM ? EPBFoundResult::Found : EPBFoundResult::NotFound;
	return VM;
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetGlobalViewModel(APlayerController* OwningPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	return GetGlobalViewModel(GetLocalPlayerFromController(OwningPlayer), ViewModelClass);
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetGlobalViewModel(const ULocalPlayer* LocalPlayer, TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	if (UPBViewModelSubsystem* VMS = GetViewModelSubsystem(LocalPlayer))
	{
		return VMS->GetGlobalViewModel(ViewModelClass);
	}
	return nullptr;
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetOrCreateActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass, EPBValidResult& Result)
{
	UPBViewModelBase* VM = GetOrCreateActorViewModel(OwningPlayer, TargetActor, ViewModelClass);
	Result = VM ? EPBValidResult::Valid : EPBValidResult::Invalid;
	return VM;
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetOrCreateActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	return GetOrCreateActorViewModel(GetLocalPlayerFromController(OwningPlayer), TargetActor, ViewModelClass);
}

UPBViewModelBase* UPBUIBlueprintLibrary::GetOrCreateActorViewModel(const ULocalPlayer* LocalPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	if (UPBViewModelSubsystem* VMS = GetViewModelSubsystem(LocalPlayer))
	{
		return VMS->GetOrCreateActorViewModel(TargetActor, ViewModelClass);
	}
	return nullptr;
}

UPBViewModelBase* UPBUIBlueprintLibrary::FindActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass, EPBFoundResult& Result)
{
	UPBViewModelBase* VM = FindActorViewModel(OwningPlayer, TargetActor, ViewModelClass);
	Result = VM ? EPBFoundResult::Found : EPBFoundResult::NotFound;
	return VM;
}

UPBViewModelBase* UPBUIBlueprintLibrary::FindActorViewModel(APlayerController* OwningPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	return FindActorViewModel(GetLocalPlayerFromController(OwningPlayer), TargetActor, ViewModelClass);
}

UPBViewModelBase* UPBUIBlueprintLibrary::FindActorViewModel(const ULocalPlayer* LocalPlayer, AActor* TargetActor, TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	if (UPBViewModelSubsystem* VMS = GetViewModelSubsystem(LocalPlayer))
	{
		return VMS->FindActorViewModel(TargetActor, ViewModelClass);
	}
	return nullptr;
}
