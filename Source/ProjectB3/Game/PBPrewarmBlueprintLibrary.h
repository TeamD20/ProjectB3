// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PBPrewarmInterface.h"
#include "PBPrewarmBlueprintLibrary.generated.h"

/**
 * 프리웜 인터페이스의 Collect 함수에서 사용하는 BP 헬퍼.
 * 하드/소프트 레퍼런스를 FPBPrewarmTargets에 누적한다.
 */
UCLASS()
class PROJECTB3_API UPBPrewarmBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// UNiagaraSystem 하드 레퍼런스를 타겟에 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Niagara",
		meta = (DisplayName = "Add Prewarm Niagara Object"))
	static void AddPrewarmNiagaraObject(
		UPARAM(ref) FPBPrewarmTargets& InTargets,
		FPBPrewarmTargets& OutTargets,
		UNiagaraSystem* NiagaraSystem);

	// SoftObjectPtr<UNiagaraSystem>을 타겟에 직접 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Niagara",
		meta = (DisplayName = "Add Prewarm Niagara Object (Soft)"))
	static void AddPrewarmNiagaraObject_Soft(
		UPARAM(ref) FPBPrewarmTargets& InTargets,
		FPBPrewarmTargets& OutTargets,
		const TSoftObjectPtr<UNiagaraSystem>& SoftNiagaraSystem);

	// USoundBase 하드 레퍼런스를 타겟에 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Sound",
		meta = (DisplayName = "Add Prewarm Sound Object"))
	static void AddPrewarmSoundObject(
		UPARAM(ref) FPBPrewarmTargets& InTargets,
		FPBPrewarmTargets& OutTargets,
		USoundBase* Sound);

	// SoftObjectPtr<USoundBase>를 타겟에 직접 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Sound",
		meta = (DisplayName = "Add Prewarm Sound Object (Soft)"))
	static void AddPrewarmSoundObject_Soft(
		UPARAM(ref) FPBPrewarmTargets& InTargets,
		FPBPrewarmTargets& OutTargets,
		const TSoftObjectPtr<USoundBase>& SoftSound);

	// 자식 오브젝트를 타겟에 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Children",
		meta = (DisplayName = "Add Prewarm Child Object"))
	static void AddPrewarmChildObject(
		UPARAM(ref) FPBPrewarmTargets& InTargets,
		FPBPrewarmTargets& OutTargets,
		UObject* ChildObject);

	// SoftObjectPtr<UObject>를 동기 로드 후 자식 타겟에 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Children",
		meta = (DisplayName = "Add Prewarm Child Object (Soft)"))
	static void AddPrewarmChildObject_Soft(
		UPARAM(ref) FPBPrewarmTargets& InTargets,
		FPBPrewarmTargets& OutTargets,
		const TSoftObjectPtr<UObject>& SoftChildObject);

	// 자식 클래스를 타겟에 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Children",
		meta = (DisplayName = "Add Prewarm Child Class"))
	static void AddPrewarmChildClass(
		UPARAM(ref) FPBPrewarmTargets& InTargets,
		FPBPrewarmTargets& OutTargets,
		UClass* ChildClass);

	// SoftClassPtr<UObject>를 동기 로드 후 자식 타겟에 추가
	UFUNCTION(BlueprintCallable, Category = "Prewarm|Children",
		meta = (DisplayName = "Add Prewarm Child Class (Soft)"))
	static void AddPrewarmChildClass_Soft(
		UPARAM(ref) FPBPrewarmTargets& InTargets,
		FPBPrewarmTargets& OutTargets,
		const TSoftClassPtr<UObject>& SoftChildClass);
};
