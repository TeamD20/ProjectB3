// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "PBCharacterPreviewComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/Combat/PBCombatSystemLibrary.h"
#include "ProjectB3/Game/PBGameplayGameMode.h"
#include "ProjectB3/Game/PBGameplayGameState.h"
#include "ProjectB3/Player/PBGameplayPlayerController.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/Player/PBPartyAIController.h"

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

	// 이 플레이어 캐릭터를 조종할 인공지능 컨트롤러 생성 및 저장
	PartyAIController = GetWorld()->SpawnActor<APBPartyAIController>(APBPartyAIController::StaticClass());

	// 방금 스폰되었거나 기본 조종 중이면
	if (PartyAIController && !GetController())
	{
		PartyAIController->Possess(this);
	}
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

void APBPlayerCharacter::UnPossessed()
{
	Super::UnPossessed();
	
	// UnPossess 시 MovementMode가 None으로 바뀌는 것을 방지
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		if (CMC->MovementMode == MOVE_None)
		{
			// 바닥 위에 있으면 Walking, 공중이면 Falling
			CMC->SetMovementMode(CMC->IsMovingOnGround() ? MOVE_Walking : MOVE_Falling);
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

	if (APBGameplayPlayerController* PC = GetController<APBGameplayPlayerController>())
	{
		PC->SetControllerMode(EPBPlayerControllerMode::TurnMovement);
	}
	
	if (APBGameplayPlayerState* PS = Cast<APBGameplayPlayerState>(UGameplayStatics::GetPlayerState(this, 0)))
	{
		if (PS->GetSelectedPartyMember() != this)
		{
			PS->SelectPartyMember(this);
		}
	}
}

void APBPlayerCharacter::OnTurnEnd()
{
	Super::OnTurnEnd();
	
	if (APBGameplayPlayerController* PC = GetController<APBGameplayPlayerController>())
	{
		PC->SetControllerMode(EPBPlayerControllerMode::None);
	}
}

void APBPlayerCharacter::HandleDeath()
{
	Super::HandleDeath();
	
	if (APBGameplayGameState* GS = GetWorld()->GetGameState<APBGameplayGameState>())
	{
		GS->NotifyPartyMemberDeath(this);
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
