// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PBCharacterPreviewComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/Combat/PBCombatSystemLibrary.h"
#include "ProjectB3/Player/PBGameplayPlayerController.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"

APBPlayerCharacter::APBPlayerCharacter()
{
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(RootComponent);
	SpringArmComponent->bUsePawnControlRotation = false;
	SpringArmComponent->bDoCollisionTest = false;
	SpringArmComponent->bInheritPitch = false;
	SpringArmComponent->bInheritYaw = false;
	SpringArmComponent->bInheritRoll = false;
	SpringArmComponent->TargetArmLength = 1500.0f;
	SpringArmComponent->SetRelativeRotation(FRotator(-55.0f, 0.0f, 0.0f));

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	CharacterPreviewComponent = CreateDefaultSubobject<UPBCharacterPreviewComponent>(TEXT("CharacterPreview"));
}

void APBPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APBPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (UPBCombatSystemLibrary::IsInCombat(this))
	{
		if (APBGameplayPlayerController* PC = GetController<APBGameplayPlayerController>())
		{
			PC->SetControllerMode(EPBPlayerControllerMode::TurnMovement);
			UpdatePathDisplayMovementRange(PC);
		}
	}
}

void APBPlayerCharacter::OnCombatBegin()
{
	Super::OnCombatBegin();

	if (APBGameplayPlayerController* PC = GetController<APBGameplayPlayerController>())
	{
		PC->SetControllerMode(EPBPlayerControllerMode::TurnMovement);
		UpdatePathDisplayMovementRange(PC);
	}
}

void APBPlayerCharacter::OnCombatEnd()
{
	if (APBGameplayPlayerController* PC = GetController<APBGameplayPlayerController>())
	{
		PC->SetControllerMode(EPBPlayerControllerMode::FreeMovement);
	}

	Super::OnCombatEnd();
}

void APBPlayerCharacter::OnTurnActivated()
{
	Super::OnTurnActivated();

	if (APBGameplayPlayerState* PS = Cast<APBGameplayPlayerState>(UGameplayStatics::GetPlayerState(this, 0)))
	{
		if (PS->GetSelectedPartyMember() != this)
		{
			PS->SelectPartyMember(this);
		}
	}
}

void APBPlayerCharacter::UpdatePathDisplayMovementRange(APBGameplayPlayerController* PC)
{
	if (IsValid(AbilitySystemComponent))
	{
		const float MovementRange = AbilitySystemComponent->GetNumericAttribute(
			UPBTurnResourceAttributeSet::GetMovementAttribute());
		PC->SetPathDisplayMovementRange(MovementRange);
	}
}
