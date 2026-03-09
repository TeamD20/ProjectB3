// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySetRegistry.h"
#include "PBAbilitySetData.h"

const UPBAbilitySetData* UPBAbilitySetRegistry::FindAbilitySetByTag(const FGameplayTag& Tag) const
{
	const TSoftObjectPtr<UPBAbilitySetData>* Found = AbilitySetMap.Find(Tag);
	if (!Found)
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilitySetRegistry: 태그 [%s]에 매핑된 DA가 없습니다."), *Tag.ToString());
		return nullptr;
	}

	if (Found->IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilitySetRegistry: 태그 [%s]의 DA 소프트 참조가 Null입니다."), *Tag.ToString());
		return nullptr;
	}

	// 미로드 시 동기 로드
	UPBAbilitySetData* LoadedDA = Found->LoadSynchronous();
	if (!IsValid(LoadedDA))
	{
		UE_LOG(LogTemp, Error, TEXT("AbilitySetRegistry: 태그 [%s]의 DA 동기 로드에 실패했습니다."), *Tag.ToString());
		return nullptr;
	}

	return LoadedDA;
}
