// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectB3/AbilitySystem/PBAbilityGrantTypes.h"
#include "ProjectB3/Game/PBPrewarmInterface.h"
#include "PBAbilitySetData.generated.h"

class UAbilitySystemComponent;

// 어빌리티 셋 정의
// DA_Innate, DA_Class_Fighter, DA_Weapon_Longsword 등
UCLASS(BlueprintType)
class PROJECTB3_API UPBAbilitySetData : public UPrimaryDataAsset, public IPBPrewarmInterface
{
	GENERATED_BODY()

public:
	// 부여할 어빌리티 목록
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<FPBAbilityGrantEntry> Abilities;

	// 부여할 패시브 GE 목록
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TArray<FPBEffectGrantEntry> PassiveEffects;

	// ASC에 어빌리티 셋을 부여하고 핸들을 반환한다.
	FPBAbilityGrantedHandles GrantToAbilitySystem(UAbilitySystemComponent* ASC, int32 CharacterLevel = 1) const;

	/*~ IPBPrewarmInterface ~*/
	virtual void CollectPrewarmChildren_Implementation(TArray<UObject*>& OutChildren) override;
};
