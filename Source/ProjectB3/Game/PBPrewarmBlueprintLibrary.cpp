// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPrewarmBlueprintLibrary.h"

void UPBPrewarmBlueprintLibrary::AddPrewarmNiagaraObject(
	TArray<TSoftObjectPtr<UNiagaraSystem>>& OutArray,
	UNiagaraSystem* NiagaraSystem)
{
	if (IsValid(NiagaraSystem))
	{
		OutArray.AddUnique(TSoftObjectPtr<UNiagaraSystem>(NiagaraSystem));
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmNiagaraObject_Soft(
	TArray<TSoftObjectPtr<UNiagaraSystem>>& OutArray,
	const TSoftObjectPtr<UNiagaraSystem>& SoftNiagaraSystem)
{
	if (!SoftNiagaraSystem.IsNull())
	{
		OutArray.AddUnique(SoftNiagaraSystem);
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmSoundObject(
	TArray<TSoftObjectPtr<USoundBase>>& OutArray,
	USoundBase* Sound)
{
	if (IsValid(Sound))
	{
		OutArray.AddUnique(TSoftObjectPtr<USoundBase>(Sound));
	}
}

void UPBPrewarmBlueprintLibrary::AddPrewarmSoundObject_Soft(
	TArray<TSoftObjectPtr<USoundBase>>& OutArray,
	const TSoftObjectPtr<USoundBase>& SoftSound)
{
	if (!SoftSound.IsNull())
	{
		OutArray.AddUnique(SoftSound);
	}
}
