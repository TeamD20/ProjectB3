// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility_Move.h"
#include "ProjectB3/AbilitySystem/Tasks/PBAbilityTask_MoveToLocation.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/Player/PBGameplayPlayerController.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/Combat/PBCombatSystemLibrary.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"

UPBGameplayAbility_Move::UPBGameplayAbility_Move()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(PBGameplayTags::Ability_Active_Move);
	SetAssetTags(AssetTags);
	
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	TriggerData.TriggerTag = PBGameplayTags::Event_Movement_MoveCommand;
	
	AbilityTriggers.Add(TriggerData);
	
	ActivationOwnedTags.AddTag(PBGameplayTags::Character_State_Moving);
}

void UPBGameplayAbility_Move::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 전투 상황이 아니면 이동자원 회복
	if (!UPBCombatSystemLibrary::IsInCombat(this))
	{
		if (UPBAbilitySystemComponent* ASC = GetPBAbilitySystemComponent())
		{
			ASC->ResetMovementResource();
		}
	}
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// AI: TriggerEventData로 즉시 이동 시작
	if (TriggerEventData && TriggerEventData->EventTag.MatchesTag(PBGameplayTags::Event_Movement_MoveCommand))
	{
		HandleMoveEvent(*TriggerEventData);
		return;
	}

	// 플레이어: PC 모드를 Movement로 전환하고 MoveCommand 이벤트 대기
	APBGameplayPlayerController* PC = GetPBPlayerController();
	if (IsValid(PC))
	{
		PC->SetControllerMode(EPBPlayerControllerMode::TurnMovement);

		// AttributeSet의 Movement 값을 PathDisplay에 전달
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (IsValid(ASC))
		{
			const float MovementRange = ASC->GetNumericAttribute(UPBTurnResourceAttributeSet::GetMovementAttribute());
			PC->SetPathDisplayMovementRange(MovementRange);
		}
	}

	UAbilityTask_WaitGameplayEvent* Task = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, PBGameplayTags::Event_Movement_MoveCommand, nullptr, true, true);
	Task->EventReceived.AddDynamic(this, &UPBGameplayAbility_Move::HandleMoveEvent);
	Task->ReadyForActivation();
}

void UPBGameplayAbility_Move::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 실행 중인 이동 Task 정리
	if (IsValid(ActiveMoveTask))
	{
		ActiveMoveTask->EndTask();
		ActiveMoveTask = nullptr;
	}

	// 전투 중인 경우 경로 포인트 기준 실제 이동 거리 차감 (반응행동 등으로 중단될 수 있으므로 실제 거리 기준)
	if (UPBCombatSystemLibrary::IsInCombat(this))
	{
		AActor* AvatarActor = GetAvatarActorFromActorInfo();
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		UPBEnvironmentSubsystem* EnvSubsystem = GetEnvironmentSubsystem();
		if (IsValid(AvatarActor) && IsValid(ASC) && IsValid(EnvSubsystem) && MovePathPoints.Num() > 0)
		{
			const float ActualDistance = EnvSubsystem->CalculateDistanceAlongPath(MovePathPoints, AvatarActor->GetActorLocation());
			if (ActualDistance > 0.0f)
			{
				ASC->ApplyModToAttribute(UPBTurnResourceAttributeSet::GetMovementAttribute(), EGameplayModOp::Additive, -ActualDistance);
			}
		}
	}
	// 전투 중이 아닌 경우 이동 자원 회복
	else
	{
		if (UPBAbilitySystemComponent* ASC = GetPBAbilitySystemComponent())
		{
			ASC->ResetMovementResource();
		}
	}
	
	MovePathPoints.Reset();

	// PC 모드 복원
	if (ActorInfo)
	{
		APBGameplayPlayerController* PC = GetPBPlayerController();
		if (IsValid(PC))
		{
			if (PC->GetControllerMode() == EPBPlayerControllerMode::Moving)
			{
				PC->EndMoving();
				PC->SetControllerMode(EPBPlayerControllerMode::TurnMovement);
			}
			
			// AttributeSet의 Movement 값을 PathDisplay에 전달
			UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
			if (IsValid(ASC))
			{
				const float MovementRange = ASC->GetNumericAttribute(UPBTurnResourceAttributeSet::GetMovementAttribute());
				PC->SetPathDisplayMovementRange(MovementRange);
			}
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPBGameplayAbility_Move::HandleMoveEvent(FGameplayEventData Payload)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo)
	{
		K2_EndAbility();
		return;
	}

	APawn* MyPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!IsValid(MyPawn))
	{
		K2_EndAbility();
		return;
	}

	// PC는 플레이어 전용 — AI인 경우 null
	APBGameplayPlayerController* PC = GetPBPlayerController();

	// Payload에서 목표 위치 추출
	const UPBTargetPayload* TargetPayload = Cast<UPBTargetPayload>(Payload.OptionalObject);
	if (!IsValid(TargetPayload))
	{
		K2_EndAbility();
		return;
	}

	const FVector HitLocation = TargetPayload->TargetData.GetSingleTargetLocation();

	UPBEnvironmentSubsystem* EnvSubsystem = GetEnvironmentSubsystem();
	if (!IsValid(EnvSubsystem))
	{
		K2_EndAbility();
		return;
	}

	AController* MoveController = MyPawn->GetController();
	if (!IsValid(MoveController))
	{
		K2_EndAbility();
		return;
	}

	const FPBPathFindResult PathCostResult = EnvSubsystem->CalculatePathForAgent(MoveController, HitLocation, false);
	if (!PathCostResult.bIsValid || PathCostResult.PathPoints.IsEmpty())
	{
		K2_EndAbility();
		return;
	}

	// AttributeSet에서 현재 남은 이동력 취득, AI는 무제한(-1)
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float MaxDist = IsValid(ASC)
		? ASC->GetNumericAttribute(UPBTurnResourceAttributeSet::GetMovementAttribute())
		: -1.0f;
	MovePathPoints.Reset();
	const FVector Destination = EnvSubsystem->CalculateClampedDestination(PathCostResult.PathPoints, MaxDist, MovePathPoints);

	// PBAbilityTask_MoveToLocation으로 NavMesh 경로 이동 — 완료 시 어빌리티 종료
	ActiveMoveTask = UPBAbilityTask_MoveToLocation::CreateTask(this, Destination);
	ActiveMoveTask->OnMoveCompleted.AddDynamic(this, &UPBGameplayAbility_Move::HandleMoveCompleted);
	ActiveMoveTask->ReadyForActivation();

	// 플레이어: Moving 모드로 전환하여 남은 경로 표시 시작
	if (IsValid(PC))
	{
		PC->BeginMoving(MovePathPoints);
	}
}

void UPBGameplayAbility_Move::HandleMoveCompleted(TEnumAsByte<EPathFollowingResult::Type> Result)
{
	ActiveMoveTask = nullptr;
	K2_EndAbility();
}

UPBEnvironmentSubsystem* UPBGameplayAbility_Move::GetEnvironmentSubsystem() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	const UGameInstance* GI = World->GetGameInstance();
	if (!IsValid(GI))
	{
		return nullptr;
	}

	return GI->GetSubsystem<UPBEnvironmentSubsystem>();
}
