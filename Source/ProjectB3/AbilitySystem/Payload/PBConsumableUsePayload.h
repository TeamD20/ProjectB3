// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBConsumableUsePayload.generated.h"

class UPBConsumableDataAsset;

/**
 * 소비 아이템 사용 이벤트 페이로드.
 * UPBTargetPayload 패턴과 동일하게 UObject 래퍼로 구현.
 * FGameplayEventData.OptionalObject를 통해 UseConsumable 어빌리티로 전달된다.
 */
UCLASS(BlueprintType)
class PROJECTB3_API UPBConsumableUsePayload : public UObject
{
	GENERATED_BODY()

public:
	// 사용할 아이템 인스턴스 ID
	UPROPERTY(BlueprintReadWrite, Category = "Consumable")
	FGuid InstanceID;

	// 소모품 데이터 에셋 (직접 참조 — 런타임 조회 비용 제거)
	UPROPERTY(BlueprintReadWrite, Category = "Consumable")
	TObjectPtr<UPBConsumableDataAsset> ConsumableDataAsset;
};
