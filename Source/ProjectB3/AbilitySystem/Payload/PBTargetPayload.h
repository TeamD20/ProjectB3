// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBTargetPayload.generated.h"

/**
 * FPBAbilityTargetData를 감싸는 UObject 래퍼.
 * FGameplayEventData.OptionalObject를 통해 어빌리티로 전달된다.
 * Payload 존재 = 타겟 결정 완료 (AI 경로 및 플레이어 None/Self 경로에서 사용).
 */
UCLASS(BlueprintType)
class PROJECTB3_API UPBTargetPayload : public UObject
{
	GENERATED_BODY()

public:
	// 타겟 데이터
	UPROPERTY(BlueprintReadWrite, Category = "Ability|Target")
	FPBAbilityTargetData TargetData;
};
