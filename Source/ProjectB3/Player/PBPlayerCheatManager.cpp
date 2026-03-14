// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPlayerCheatManager.h"
#include "PBGameplayPlayerController.h"
#include "PBGameplayPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"

void UPBPlayerCheatManager::EnterMovementMode()
{
	APBGameplayPlayerController* PC = Cast<APBGameplayPlayerController>(GetOuterAPlayerController());
	if (!IsValid(PC))
	{
		return;
	}

	APawn* MyPawn = PC->GetPawn();
	if (!IsValid(MyPawn))
	{
		return;
	}

	// 이동 어빌리티 발동 — ActivateAbility 내부에서 PC를 Movement 모드로 전환
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyPawn);
	if (!IsValid(ASC))
	{
		return;
	}

	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(PBGameplayTags::Ability_Active_Move);
	ASC->TryActivateAbilitiesByTag(TagContainer);
}

void UPBPlayerCheatManager::EnterFreeMovementMode()
{
	APBGameplayPlayerController* PC = Cast<APBGameplayPlayerController>(GetOuterAPlayerController());
	if (!IsValid(PC))
	{
		return;
	}

	PC->SetControllerMode(EPBPlayerControllerMode::FreeMovement);
}

void UPBPlayerCheatManager::ExitMode()
{
	APBGameplayPlayerController* PC = Cast<APBGameplayPlayerController>(GetOuterAPlayerController());
	if (!IsValid(PC))
	{
		return;
	}

	PC->ExitCurrentMode();
}

void UPBPlayerCheatManager::SetMovement(float Value)
{
	APBGameplayPlayerController* PC = Cast<APBGameplayPlayerController>(GetOuterAPlayerController());
	if (!IsValid(PC))
	{
		return;
	}

	APawn* MyPawn = PC->GetPawn();
	if (!IsValid(MyPawn))
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyPawn);
	if (!IsValid(ASC))
	{
		return;
	}

	ASC->SetNumericAttributeBase(UPBTurnResourceAttributeSet::GetMovementAttribute(), Value);
	if (PC->GetControllerMode() == EPBPlayerControllerMode::TurnMovement)
	{
		PC->ClearPathDisplay();
		PC->SetPathDisplayMovementRange(Value);	
	}
}

void UPBPlayerCheatManager::SelectPartyMember(int32 Index)
{
	APlayerController* PC = GetOuterAPlayerController();
	if (!IsValid(PC))
	{
		return;
	}

	APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>();
	if (!IsValid(PS))
	{
		return;
	}

	const TArray<AActor*> Members = PS->GetPartyMembers();
	if (!Members.IsValidIndex(Index))
	{
		return;
	}

	PS->SelectPartyMember(Members[Index]);
}
