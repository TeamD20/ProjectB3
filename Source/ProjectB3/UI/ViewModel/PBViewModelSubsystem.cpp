// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBViewModelSubsystem.h"
#include "PBViewModelBase.h"

void UPBViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPBViewModelSubsystem::Deinitialize()
{
	ResetSystem();
	Super::Deinitialize();
}

void UPBViewModelSubsystem::ResetSystem()
{
	// Global ViewModel 정리
	for (auto& [Class, VM] : GlobalViewModelMap)
	{
		if (IsValid(VM))
		{
			VM->Deinitialize();
		}
	}
	GlobalViewModelMap.Empty();

	// Actor-Bound ViewModel 정리
	for (auto& [Key, VM] : ActorViewModelMap)
	{
		if (IsValid(VM))
		{
			VM->Deinitialize();
		}
	}
	ActorViewModelMap.Empty();
	BoundActors.Empty();
}

UPBViewModelBase* UPBViewModelSubsystem::RegisterGlobalViewModel(TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	if (!ViewModelClass)
	{
		return nullptr;
	}

	if (TObjectPtr<UPBViewModelBase>* Found = GlobalViewModelMap.Find(ViewModelClass))
	{
		return *Found;
	}

	UPBViewModelBase* NewViewModel = NewObject<UPBViewModelBase>(this, ViewModelClass);
	if (NewViewModel)
	{
		NewViewModel->InitializeForPlayer(GetLocalPlayer());
		GlobalViewModelMap.Add(ViewModelClass, NewViewModel);
	}

	return NewViewModel;
}

UPBViewModelBase* UPBViewModelSubsystem::GetGlobalViewModel(TSubclassOf<UPBViewModelBase> ViewModelClass) const
{
	if (const TObjectPtr<UPBViewModelBase>* Found = GlobalViewModelMap.Find(ViewModelClass))
	{
		return *Found;
	}
	return nullptr;
}

UPBViewModelBase* UPBViewModelSubsystem::GetOrCreateGlobalViewModel(TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	if (UPBViewModelBase* Existing = GetGlobalViewModel(ViewModelClass))
	{
		return Existing;
	}
	return RegisterGlobalViewModel(ViewModelClass);
}

bool UPBViewModelSubsystem::UnregisterGlobalViewModel(TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	TObjectPtr<UPBViewModelBase> Removed;
	if (GlobalViewModelMap.RemoveAndCopyValue(ViewModelClass, Removed))
	{
		if (Removed)
		{
			Removed->Deinitialize();
		}
		return true;
	}
	return false;
}

UPBViewModelBase* UPBViewModelSubsystem::GetOrCreateActorViewModel(
	AActor* TargetActor,
	TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	if (!TargetActor || !ViewModelClass)
	{
		return nullptr;
	}

	// 기존 검색
	FPBActorViewModelKey Key(TargetActor, ViewModelClass);
	if (TObjectPtr<UPBViewModelBase>* Found = ActorViewModelMap.Find(Key))
	{
		return *Found;
	}

	// 새로 생성
	UPBViewModelBase* NewViewModel = NewObject<UPBViewModelBase>(this, ViewModelClass);
	if (NewViewModel)
	{
		NewViewModel->InitializeForActor(TargetActor, GetLocalPlayer());
		ActorViewModelMap.Add(Key, NewViewModel);

		// Actor 파괴 시 자동 정리 (한 번만 바인딩)
		if (!BoundActors.Contains(TargetActor))
		{
			TargetActor->OnDestroyed.AddDynamic(this, &UPBViewModelSubsystem::HandleActorDestroyed);
			BoundActors.Add(TargetActor);
		}
	}

	return NewViewModel;
}

UPBViewModelBase* UPBViewModelSubsystem::FindActorViewModel(
	AActor* TargetActor,
	TSubclassOf<UPBViewModelBase> ViewModelClass) const
{
	if (!TargetActor || !ViewModelClass)
	{
		return nullptr;
	}

	FPBActorViewModelKey Key(TargetActor, ViewModelClass);
	if (const TObjectPtr<UPBViewModelBase>* Found = ActorViewModelMap.Find(Key))
	{
		return *Found;
	}
	return nullptr;
}

bool UPBViewModelSubsystem::RemoveActorViewModel(
	AActor* TargetActor,
	TSubclassOf<UPBViewModelBase> ViewModelClass)
{
	if (!TargetActor || !ViewModelClass)
	{
		return false;
	}

	FPBActorViewModelKey Key(TargetActor, ViewModelClass);
	TObjectPtr<UPBViewModelBase> Removed;
	if (ActorViewModelMap.RemoveAndCopyValue(Key, Removed))
	{
		if (Removed)
		{
			Removed->Deinitialize();
		}
		return true;
	}
	return false;
}

void UPBViewModelSubsystem::RemoveAllViewModelsForActor(AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	for (auto It = ActorViewModelMap.CreateIterator(); It; ++It)
	{
		if (It.Key().Actor == TargetActor)
		{
			if (It.Value())
			{
				It.Value()->Deinitialize();
			}
			It.RemoveCurrent();
		}
	}

	BoundActors.Remove(TargetActor);
}

void UPBViewModelSubsystem::HandleActorDestroyed(AActor* DestroyedActor)
{
	RemoveAllViewModelsForActor(DestroyedActor);
}

UPBViewModelBase* UPBViewModelSubsystem::FindGlobalViewModelByTag(FGameplayTag ViewModelTag) const
{
	if (!ViewModelTag.IsValid())
	{
		return nullptr;
	}

	for (const auto& [Class, VM] : GlobalViewModelMap)
	{
		if (IsValid(VM) && VM->GetViewModelTag().MatchesTagExact(ViewModelTag))
		{
			return VM;
		}
	}

	return nullptr;
}

void UPBViewModelSubsystem::FindActorViewModelsByTag(FGameplayTag ViewModelTag, TArray<UPBViewModelBase*>& OutViewModels) const
{
	OutViewModels.Reset();

	if (!ViewModelTag.IsValid())
	{
		return;
	}

	for (const auto& [Key, VM] : ActorViewModelMap)
	{
		if (IsValid(VM) && VM->GetViewModelTag().MatchesTagExact(ViewModelTag))
		{
			OutViewModels.Add(VM);
		}
	}
}

bool UPBViewModelSubsystem::SetVisibilityByTag(FGameplayTag ViewModelTag, bool bNewVisible, bool bAffectActorBound)
{
	if (!ViewModelTag.IsValid())
	{
		return false;
	}

	bool bHasChanged = false;

	// Global ViewModel
	if (UPBViewModelBase* GlobalVM = FindGlobalViewModelByTag(ViewModelTag))
	{
		const bool bPrevVisible = GlobalVM->IsVisible();
		GlobalVM->SetVisibilityOverride(bNewVisible);
		bHasChanged |= (bPrevVisible != bNewVisible);
	}

	// Actor-Bound ViewModel
	if (bAffectActorBound)
	{
		TArray<UPBViewModelBase*> ActorVMs;
		FindActorViewModelsByTag(ViewModelTag, ActorVMs);

		for (UPBViewModelBase* VM : ActorVMs)
		{
			const bool bPrevVisible = VM->IsVisible();
			VM->SetVisibilityOverride(bNewVisible);
			bHasChanged |= (bPrevVisible != bNewVisible);
		}
	}

	return bHasChanged;
}

bool UPBViewModelSubsystem::GetVisibilityByTag(FGameplayTag ViewModelTag, bool bDefaultVisible) const
{
	if (!ViewModelTag.IsValid())
	{
		return bDefaultVisible;
	}

	if (UPBViewModelBase* GlobalVM = FindGlobalViewModelByTag(ViewModelTag))
	{
		return GlobalVM->IsVisible();
	}

	TArray<UPBViewModelBase*> ActorVMs;
	FindActorViewModelsByTag(ViewModelTag, ActorVMs);

	for (UPBViewModelBase* VM : ActorVMs)
	{
		return VM->IsVisible();
	}

	return bDefaultVisible;
}

void UPBViewModelSubsystem::RestoreVisibilityByTag(FGameplayTag ViewModelTag, bool bVisible, bool bAffectActorBound)
{
	if (!ViewModelTag.IsValid())
	{
		return;
	}

	if (UPBViewModelBase* GlobalVM = FindGlobalViewModelByTag(ViewModelTag))
	{
		GlobalVM->ClearVisibilityOverride();
		GlobalVM->SetDesiredVisibility(bVisible);
	}

	if (bAffectActorBound)
	{
		TArray<UPBViewModelBase*> ActorVMs;
		FindActorViewModelsByTag(ViewModelTag, ActorVMs);

		for (UPBViewModelBase* VM : ActorVMs)
		{
			VM->ClearVisibilityOverride();
			VM->SetDesiredVisibility(bVisible);
		}
	}
}
