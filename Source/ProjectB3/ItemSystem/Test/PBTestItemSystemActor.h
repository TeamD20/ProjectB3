// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "PBTestItemSystemActor.generated.h"

class UAbilitySystemComponent;
class UPBInventoryComponent;
class UPBEquipmentComponent;

/**
 * 아이템 시스템 테스트용 더미 액터.
 * IAbilitySystemInterface를 구현하여 장비 GAS 부여를 검증한다.
 */
UCLASS()
class PROJECTB3_API APBTestItemSystemActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APBTestItemSystemActor();

	/*~ IAbilitySystemInterface Interface ~*/
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPBInventoryComponent> InventoryComponent;

	UPROPERTY()
	TObjectPtr<UPBEquipmentComponent> EquipmentComponent;
};
