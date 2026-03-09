// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "ActiveGameplayEffectHandle.h"
#include "Abilities/PBGameplayAbility.h"
#include "PBAbilityGrantTypes.generated.h"

class UPBGameplayAbility;
class UGameplayEffect;

// 부여할 어빌리티 1개 정의
USTRUCT(BlueprintType)
struct FPBAbilityGrantEntry
{
	GENERATED_BODY()

	// 부여할 어빌리티 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UPBGameplayAbility> AbilityClass;

	// 런타임에 Spec에 추가할 동적 태그 (출처 식별, UI 필터링 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer DynamicTags;

	// 어빌리티 레벨
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 AbilityLevel = 1;

	// 해금 조건 레벨 (0이면 즉시, 클래스 어빌리티용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 RequiredLevel = 0;

	// 유효성 검증
	bool IsValidData() const
	{
		return AbilityClass != nullptr;
	}
};

// 부여할 패시브 GE 1개 정의
USTRUCT(BlueprintType)
struct FPBEffectGrantEntry
{
	GENERATED_BODY()

	// 부여할 GE 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	// 유효성 검증
	bool IsValidData() const
	{
		return EffectClass != nullptr;
	}
};

// SourceTag별 부여된 핸들 래퍼
USTRUCT()
struct FPBSourceGrantedHandles
{
	GENERATED_BODY()

	// 부여된 어빌리티 핸들
	TArray<FGameplayAbilitySpecHandle> AbilityHandles;

	// 부여된 GE 핸들
	TArray<FActiveGameplayEffectHandle> EffectHandles;
};
