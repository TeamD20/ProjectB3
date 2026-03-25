// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPrewarmManagerSubsystem.h"
#include "PBPrewarmInterface.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBPrewarm, Log, All);

namespace PBPrewarmConstants
{
	// 프리웜 실행 위치 (월드 밖)
	static const FVector PrewarmLocation(1000000.0, 1000000.0, -1000000.0);

	// 프리웜 대상 스케일 (극소화)
	static const FVector PrewarmScale(0.01, 0.01, 0.01);
}

void UPBPrewarmManagerSubsystem::ExecutePrewarm(const TArray<UObject*>& RootObjects)
{
	TSet<UObject*> Visited;
	TArray<TSoftObjectPtr<UNiagaraSystem>> NiagaraAssets;
	TArray<TSoftObjectPtr<USoundBase>> SoundAssets;

	// 1. 재귀 수집
	for (UObject* Root : RootObjects)
	{
		CollectFromObject(Root, Visited, NiagaraAssets, SoundAssets);
	}

	UE_LOG(LogPBPrewarm, Display, TEXT("프리웜 수집 완료: Niagara=%d, Sound=%d"),
		NiagaraAssets.Num(), SoundAssets.Num());

	// 2. Niagara 프리웜
	for (const TSoftObjectPtr<UNiagaraSystem>& Soft : NiagaraAssets)
	{
		if (UNiagaraSystem* NS = Soft.LoadSynchronous())
		{
			PrewarmNiagaraAsset(NS);
		}
		else
		{
			UE_LOG(LogPBPrewarm, Warning, TEXT("Niagara 에셋 로드 실패: %s"), *Soft.GetAssetName());
		}
	}

	// 3. Sound 프리웜
	for (const TSoftObjectPtr<USoundBase>& Soft : SoundAssets)
	{
		if (USoundBase* Sound = Soft.LoadSynchronous())
		{
			PrewarmSoundAsset(Sound);
		}
		else
		{
			UE_LOG(LogPBPrewarm, Warning, TEXT("Sound 에셋 로드 실패: %s"), *Soft.GetAssetName());
		}
	}

	UE_LOG(LogPBPrewarm, Display, TEXT("프리웜 실행 완료. 총 프리웜 이력: %d"), PrewarmedAssets.Num());
}

void UPBPrewarmManagerSubsystem::ResetPrewarmHistory()
{
	PrewarmedAssets.Empty();
	UE_LOG(LogPBPrewarm, Display, TEXT("프리웜 이력 초기화 완료."));
}

void UPBPrewarmManagerSubsystem::CollectFromObject(
	UObject* Object,
	TSet<UObject*>& Visited,
	TArray<TSoftObjectPtr<UNiagaraSystem>>& OutNiagaraAssets,
	TArray<TSoftObjectPtr<USoundBase>>& OutSoundAssets)
{
	if (!IsValid(Object) || Visited.Contains(Object))
	{
		return;
	}
	Visited.Add(Object);

	if (!Object->GetClass()->ImplementsInterface(UPBPrewarmInterface::StaticClass()))
	{
		return;
	}

	// 타입별 Collect 호출 (BlueprintNativeEvent → BP 오버라이드 자동 실행)
	IPBPrewarmInterface::Execute_CollectPrewarmNiagaraAssets(Object, OutNiagaraAssets);
	IPBPrewarmInterface::Execute_CollectPrewarmSoundAssets(Object, OutSoundAssets);

	// 자식 재귀 수집
	TArray<UObject*> Children;
	IPBPrewarmInterface::Execute_CollectPrewarmChildren(Object, Children);
	for (UObject* Child : Children)
	{
		CollectFromObject(Child, Visited, OutNiagaraAssets, OutSoundAssets);
	}
}

void UPBPrewarmManagerSubsystem::PrewarmNiagaraAsset(UNiagaraSystem* System)
{
	if (!IsValid(System))
	{
		return;
	}

	const FSoftObjectPath AssetPath(System);
	if (PrewarmedAssets.Contains(AssetPath))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 월드 밖 극단적 위치에 극소 스케일로 스폰 → GPU 셰이더 컴파일 유도
	UNiagaraComponent* Comp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		System,
		PBPrewarmConstants::PrewarmLocation,
		FRotator::ZeroRotator,
		PBPrewarmConstants::PrewarmScale,
		true,  // bAutoDestroy
		true,  // bAutoActivate
		ENCPoolMethod::None);

	if (IsValid(Comp))
	{
		Comp->Deactivate();
	}

	PrewarmedAssets.Add(AssetPath);
	UE_LOG(LogPBPrewarm, Verbose, TEXT("Niagara 프리웜 완료: %s"), *System->GetName());
}

void UPBPrewarmManagerSubsystem::PrewarmSoundAsset(USoundBase* Sound)
{
	if (!IsValid(Sound))
	{
		return;
	}

	const FSoftObjectPath AssetPath(Sound);
	if (PrewarmedAssets.Contains(AssetPath))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	// 무음 재생으로 메모리에 사운드 데이터 적재
	UGameplayStatics::PlaySound2D(World, Sound, 0.f);

	PrewarmedAssets.Add(AssetPath);
	UE_LOG(LogPBPrewarm, Verbose, TEXT("Sound 프리웜 완료: %s"), *Sound->GetName());
}
