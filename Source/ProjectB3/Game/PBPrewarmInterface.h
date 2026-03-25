// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "PBPrewarmInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPBPrewarmInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 프리웜 대상 객체가 구현하는 인터페이스.
 * 재귀적 에셋 수집 체인을 통해 Niagara/Sound 에셋을 사전 로딩한다.
 */
class PROJECTB3_API IPBPrewarmInterface
{
	GENERATED_BODY()

public:
	// Niagara 에셋 수집. OutAssets에 프리웜할 나이아가라 시스템을 추가.
	UFUNCTION(BlueprintNativeEvent, Category = "Prewarm")
	void CollectPrewarmNiagaraAssets(UPARAM(ref) TArray<TSoftObjectPtr<UNiagaraSystem>>& OutAssets);

	// Sound 에셋 수집. OutAssets에 프리웜할 사운드를 추가.
	UFUNCTION(BlueprintNativeEvent, Category = "Prewarm")
	void CollectPrewarmSoundAssets(UPARAM(ref) TArray<TSoftObjectPtr<USoundBase>>& OutAssets);

	// 자식 객체 수집 (재귀 탐색용). OutChildren에 프리웜 인터페이스를 구현한 자식 객체를 추가.
	UFUNCTION(BlueprintNativeEvent, Category = "Prewarm")
	void CollectPrewarmChildren(UPARAM(ref) TArray<UObject*>& OutChildren);
};
