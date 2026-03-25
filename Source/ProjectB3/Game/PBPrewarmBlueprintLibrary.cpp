// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPrewarmBlueprintLibrary.h"

void UPBPrewarmBlueprintLibrary::AddPrewarmNiagaraObject(
	FPBPrewarmTargets& InTargets,
	FPBPrewarmTargets& OutTargets,
	UNiagaraSystem* NiagaraSystem)
{
	OutTargets = InTargets;
	if (IsValid(NiagaraSystem))
	{
		OutTargets.NiagaraAssets.AddUnique(TSoftObjectPtr<UNiagaraSystem>(NiagaraSystem));
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmNiagaraObject_Soft(
	FPBPrewarmTargets& InTargets,
	FPBPrewarmTargets& OutTargets,
	const TSoftObjectPtr<UNiagaraSystem>& SoftNiagaraSystem)
{
	OutTargets = InTargets;
	if (!SoftNiagaraSystem.IsNull())
	{
		OutTargets.NiagaraAssets.AddUnique(SoftNiagaraSystem);
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmSoundObject(
	FPBPrewarmTargets& InTargets,
	FPBPrewarmTargets& OutTargets,
	USoundBase* Sound)
{
	OutTargets = InTargets;
	if (IsValid(Sound))
	{
		OutTargets.SoundAssets.AddUnique(TSoftObjectPtr<USoundBase>(Sound));
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmSoundObject_Soft(
	FPBPrewarmTargets& InTargets,
	FPBPrewarmTargets& OutTargets,
	const TSoftObjectPtr<USoundBase>& SoftSound)
{
	OutTargets = InTargets;
	if (!SoftSound.IsNull())
	{
		OutTargets.SoundAssets.AddUnique(SoftSound);
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmChildObject(
	FPBPrewarmTargets& InTargets,
	FPBPrewarmTargets& OutTargets,
	UObject* ChildObject)
{
	OutTargets = InTargets;
	if (IsValid(ChildObject))
	{
		OutTargets.Children.AddUnique(ChildObject);
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmChildObject_Soft(
	FPBPrewarmTargets& InTargets,
	FPBPrewarmTargets& OutTargets,
	const TSoftObjectPtr<UObject>& SoftChildObject)
{
	OutTargets = InTargets;
	if (!SoftChildObject.IsNull())
	{
		if (UObject* LoadedObject = SoftChildObject.LoadSynchronous())
		{
			OutTargets.Children.AddUnique(LoadedObject);
		}
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmChildClass(
	FPBPrewarmTargets& InTargets,
	FPBPrewarmTargets& OutTargets,
	UClass* ChildClass)
{
	OutTargets = InTargets;
	if (IsValid(ChildClass))
	{
		OutTargets.Children.AddUnique(ChildClass);
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmChildClass_Soft(
	FPBPrewarmTargets& InTargets,
	FPBPrewarmTargets& OutTargets,
	const TSoftClassPtr<UObject>& SoftChildClass)
{
	OutTargets = InTargets;
	if (!SoftChildClass.IsNull())
	{
		if (UClass* LoadedClass = SoftChildClass.LoadSynchronous())
		{
			OutTargets.Children.AddUnique(LoadedClass);
		}
	}
}
