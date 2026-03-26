// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PBPrewarmInterface.h"
#include "PBPrewarmManagerSubsystem.generated.h"

class UNiagaraSystem;
class USoundBase;

/**
 * 프리웜 매니저 서브시스템.
 * IPBPrewarmInterface 구현 객체들로부터 에셋을 재귀 수집하고 사전 로딩한다.
 */
UCLASS()
class PROJECTB3_API UPBPrewarmManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 루트 객체들로부터 재귀 수집 후 프리웜 실행
	void ExecutePrewarm(const TArray<UObject*>& RootObjects);

	// 프리웜 이력 초기화
	void ResetPrewarmHistory();

private:
	// IPBPrewarmInterface 구현 객체로부터 재귀 에셋 수집
	void CollectFromObject(UObject* Object, TSet<const UObject*>& Visited,
		FPBPrewarmTargets& OutTargets);

	// Niagara 에셋 프리웜 — 월드 밖에서 스폰 후 다음 프레임에 비활성화
	void PrewarmNiagaraAsset(UNiagaraSystem* System);

	// Sound 에셋 프리웜 — 무음 재생으로 메모리 적재
	void PrewarmSoundAsset(USoundBase* Sound);

private:
	// 이미 프리웜된 에셋 경로 (중복 방지)
	TSet<FSoftObjectPath> PrewarmedAssets;
};
