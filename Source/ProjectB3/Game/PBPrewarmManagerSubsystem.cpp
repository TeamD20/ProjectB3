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
	TSet<const UObject*> Visited;
	FPBPrewarmTargets PrewarmTargets;

	// 1. 재귀 수집
	for (UObject* Root : RootObjects)
	{
		CollectFromObject(Root, Visited, PrewarmTargets);
	}

	UE_LOG(LogPBPrewarm, Display, TEXT("프리웜 수집 완료: Niagara=%d, Sound=%d"),
		PrewarmTargets.NiagaraAssets.Num(), PrewarmTargets.SoundAssets.Num());

	// 2. Niagara 프리웜
	for (const TSoftObjectPtr<UNiagaraSystem>& Soft : PrewarmTargets.NiagaraAssets)
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
	for (const TSoftObjectPtr<USoundBase>& Soft : PrewarmTargets.SoundAssets)
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
	TSet<const UObject*>& Visited,
	FPBPrewarmTargets& OutTargets)
{
	if (!IsValid(Object))
	{
		return;
	}

	UObject* TargetObject = Object;
	if (UClass* TargetClass = Cast<UClass>(Object))
	{
		if (Visited.Contains(TargetClass))
		{
			return;
		}
		Visited.Add(TargetClass);

		TargetObject = TargetClass->GetDefaultObject();
		if (!IsValid(TargetObject) || Visited.Contains(TargetObject))
		{
			return;
		}
		Visited.Add(TargetObject);
	}
	else
	{
		if (Visited.Contains(Object))
		{
			return;
		}
		Visited.Add(Object);
	}

	if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(TargetObject))
	{
		OutTargets.NiagaraAssets.AddUnique(TSoftObjectPtr<UNiagaraSystem>(NiagaraSystem));
		return;
	}

	if (USoundBase* Sound = Cast<USoundBase>(TargetObject))
	{
		OutTargets.SoundAssets.AddUnique(TSoftObjectPtr<USoundBase>(Sound));
		return;
	}

	if (!TargetObject->GetClass()->ImplementsInterface(UPBPrewarmInterface::StaticClass()))
	{
		return;
	}

	// C++ 구현 수집
	FPBPrewarmTargets CollectedTargets;
	if (IPBPrewarmInterface* PrewarmInterface = Cast<IPBPrewarmInterface>(TargetObject))
	{
		PrewarmInterface->NativeCollectPrewarmTargets(CollectedTargets);
	}

	// BP 구현 수집 (In -> Return)
	FPBPrewarmTargets BlueprintCollectedTargets = IPBPrewarmInterface::Execute_CollectPrewarmTargets(TargetObject, CollectedTargets);

	for (const TSoftObjectPtr<UNiagaraSystem>& Asset : BlueprintCollectedTargets.NiagaraAssets)
	{
		if (!Asset.IsNull())
		{
			CollectedTargets.NiagaraAssets.AddUnique(Asset);
		}
	}

	for (const TSoftObjectPtr<USoundBase>& Asset : BlueprintCollectedTargets.SoundAssets)
	{
		if (!Asset.IsNull())
		{
			CollectedTargets.SoundAssets.AddUnique(Asset);
		}
	}

	for (UObject* Child : BlueprintCollectedTargets.Children)
	{
		if (IsValid(Child))
		{
			CollectedTargets.Children.AddUnique(Child);
		}
	}

	for (const TSoftObjectPtr<UNiagaraSystem>& Asset : CollectedTargets.NiagaraAssets)
	{
		if (!Asset.IsNull())
		{
			OutTargets.NiagaraAssets.AddUnique(Asset);
		}
	}

	for (const TSoftObjectPtr<USoundBase>& Asset : CollectedTargets.SoundAssets)
	{
		if (!Asset.IsNull())
		{
			OutTargets.SoundAssets.AddUnique(Asset);
		}
	}

	// 자식 재귀 수집
	for (UObject* Child : CollectedTargets.Children)
	{
		CollectFromObject(Child, Visited, OutTargets);
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
		// Loop VFX 대비: 다음 프레임에 비활성화 후 파괴
		TWeakObjectPtr<UNiagaraComponent> WeakComp = Comp;
		World->GetTimerManager().SetTimerForNextTick([WeakComp]()
		{
			if (WeakComp.IsValid())
			{
				WeakComp->Deactivate();
				WeakComp->DestroyComponent();
			}
		});
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
