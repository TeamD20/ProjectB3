// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PBCharacterPreviewComponent.h"

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
