// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "PBPrewarmInterface.generated.h"

USTRUCT(BlueprintType)
struct FPBPrewarmTargets
{
	GENERATED_BODY()

	// 프리웜할 나이아가라 에셋 목록
	UPROPERTY(BlueprintReadWrite, Category = "Prewarm")
	TArray<TSoftObjectPtr<UNiagaraSystem>> NiagaraAssets;

	// 프리웜할 사운드 에셋 목록
	UPROPERTY(BlueprintReadWrite, Category = "Prewarm")
	TArray<TSoftObjectPtr<USoundBase>> SoundAssets;

	// 재귀 수집할 자식 오브젝트 목록
	UPROPERTY(BlueprintReadWrite, Category = "Prewarm")
	TArray<UObject*> Children;
};

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
	// BP 구현용 프리웜 타겟 수집. InTargets를 기반으로 결과 타겟을 반환한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Prewarm")
	FPBPrewarmTargets CollectPrewarmTargets(UPARAM(ref) FPBPrewarmTargets& InTargets);

	// C++ 구현용 프리웜 타겟 수집. InOutTargets에 누적한다.
	virtual void NativeCollectPrewarmTargets(FPBPrewarmTargets& InOutTargets) {}
};
