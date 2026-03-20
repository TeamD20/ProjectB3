// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PBCombatSystemLibrary.generated.h"

class UPBCombatManagerSubsystem;
/**
 * 
 */
UCLASS()
class PROJECTB3_API UPBCombatSystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static UPBCombatManagerSubsystem* GetCombatManager(UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject"))
	static bool IsInCombat(UObject* WorldContextObject);
	
	UFUNCTION(BlueprintCallable)
	static bool IsMyTurn(AActor* Combatant);
};
