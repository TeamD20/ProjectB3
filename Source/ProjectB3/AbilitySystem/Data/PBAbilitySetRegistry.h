// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PBAbilitySetRegistry.generated.h"

class UPBAbilitySetData;

// 태그 : DA 중앙 매핑 레지스트리 (세이브/로드, 리스펙 등에서 사용)
// TSoftObjectPtr로 참조 — 레지스트리 로드 시 DA를 연쇄 로드하지 않음
UCLASS(BlueprintType)
class PROJECTB3_API UPBAbilitySetRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 태그로 DA를 동기 로드하여 반환 (미로드 시 LoadSynchronous)
	const UPBAbilitySetData* FindAbilitySetByTag(const FGameplayTag& Tag) const;

protected:
	// 레지스트리 엔트리 (태그 → DA 소프트 참조)
	UPROPERTY(EditDefaultsOnly, Category = "Registry")
	TMap<FGameplayTag, TSoftObjectPtr<UPBAbilitySetData>> AbilitySetMap;
};
