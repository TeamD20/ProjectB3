// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PBAbilityGrantTypes.h"
#include "PBAbilitySystemComponent.generated.h"

class UPBAbilitySetData;

DECLARE_LOG_CATEGORY_EXTERN(LogPBAbilitySystem, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTB3_API UPBAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UPBAbilitySystemComponent();

	// DA 기반 어빌리티 일괄 부여.
	void GrantAbilitiesFromData(
		const FGameplayTag& SourceTag,
		const UPBAbilitySetData* Data,
		int32 CharacterLevel = 1);

	// 특정 출처의 어빌리티 일괄 제거
	void RemoveAbilitiesBySource(const FGameplayTag& SourceTag);

	// GrantedHandleMap에 등록된 전체 어빌리티 제거
	void RemoveAllSourceGrantedAbilities();

	// 특정 출처에 부여된 핸들이 있는지 확인
	bool HasAbilitiesFromSource(const FGameplayTag& SourceTag) const;

	// 포함/제외 태그 조건으로 활성화 가능한 어빌리티 Spec 핸들 조회
	TArray<FGameplayAbilitySpecHandle> GetAbilitySpecHandlesByTagFilter(
		const FGameplayTagContainer& RequireTags,
		const FGameplayTagContainer& IgnoreTags = FGameplayTagContainer()) const;

protected:
	// 출처별 핸들 캐시
	TMap<FGameplayTag, FPBSourceGrantedHandles> GrantedHandleMap;
};
