// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBItemDataAsset.h"
#include "Abilities/GameplayAbility.h"
#include "PBConsumableDataAsset.generated.h"

class UPBGameplayAbility;
/**
 * 소비 아이템 전용 DataAsset.
 * UPBItemDataAsset의 소모품 UI 필드를 그대로 활용하고,
 * 사용 시 임시 부여·발동할 어빌리티 클래스를 추가한다.
 * DA 네이밍: DA_Consumable_HealthPotion 등.
 */
UCLASS(BlueprintType)
class PROJECTB3_API UPBConsumableDataAsset : public UPBItemDataAsset
{
	GENERATED_BODY()

public:
	// 사용 시 GiveAbility + TryActivateAbility로 일회성 발동할 어빌리티 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable")
	TSubclassOf<UPBGameplayAbility> ConsumableAbilityClass;
};
