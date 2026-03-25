// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "PBPrewarmBlueprintLibrary.generated.h"

/**
 * 프리웜 인터페이스의 Collect 함수에서 사용하는 BP 헬퍼.
 * 하드 레퍼런스 → SoftObjectPtr 변환 및 배열 추가를 지원한다.
 */
UCLASS()
class PROJECTB3_API UPBPrewarmBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/*------- Niagara -------*/

	// UNiagaraSystem 하드 레퍼런스를 SoftObjectPtr로 변환하여 배열에 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Niagara",
		meta = (DisplayName = "Add Prewarm Niagara Object"))
	static void AddPrewarmNiagaraObject(
		UPARAM(ref) TArray<TSoftObjectPtr<UNiagaraSystem>>& OutArray,
		UNiagaraSystem* NiagaraSystem);

	// SoftObjectPtr<UNiagaraSystem>을 배열에 직접 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Niagara",
		meta = (DisplayName = "Add Prewarm Niagara Object (Soft)"))
	static void AddPrewarmNiagaraObject_Soft(
		UPARAM(ref) TArray<TSoftObjectPtr<UNiagaraSystem>>& OutArray,
		const TSoftObjectPtr<UNiagaraSystem>& SoftNiagaraSystem);

	/*------- Sound -------*/

	// USoundBase 하드 레퍼런스를 SoftObjectPtr로 변환하여 배열에 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Sound",
		meta = (DisplayName = "Add Prewarm Sound Object"))
	static void AddPrewarmSoundObject(
		UPARAM(ref) TArray<TSoftObjectPtr<USoundBase>>& OutArray,
		USoundBase* Sound);

	// SoftObjectPtr<USoundBase>을 배열에 직접 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Sound",
		meta = (DisplayName = "Add Prewarm Sound Object (Soft)"))
	static void AddPrewarmSoundObject_Soft(
		UPARAM(ref) TArray<TSoftObjectPtr<USoundBase>>& OutArray,
		const TSoftObjectPtr<USoundBase>& SoftSound);
};
