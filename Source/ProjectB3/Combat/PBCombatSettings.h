// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (전투 시스템 기본 규칙 작성)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PBCombatSettings.generated.h"

/**
 * 
 */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Combat System Settings"))
class PROJECTB3_API UPBCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPBCombatSettings();
	
public:
	UPROPERTY(config, EditAnywhere, Category = "Combat|Faction")
	FColor FriendlyColor = FColor::Cyan;
	
	UPROPERTY(config, EditAnywhere, Category = "Combat|Faction")
	FColor HostileColor = FColor::Red;
	
	UPROPERTY(config, EditAnywhere, Category = "Combat|Faction")
	FColor NeutralColor = FColor::Yellow;
};
