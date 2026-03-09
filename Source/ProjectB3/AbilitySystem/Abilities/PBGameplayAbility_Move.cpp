// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility_Move.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/Player/PBGameplayPlayerController.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/PBGameplayTags.h"

UPBGameplayAbility_Move::UPBGameplayAbility_Move()
{
	// 이동 어빌리티는 다른 어빌리티에 의해 캔슬될 때까지 유지
	EndMode = EPBAbilityEndMode::Manual;
}

void UPBGameplayAbility_Move::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// PC 모드를 Movement로 전환하고 PathDisplay 이동 범위 전달
	APBGameplayPlayerController* PC = Cast<APBGameplayPlayerController>(ActorInfo->PlayerController.Get());
	if (IsValid(PC))
	{
		PC->SetControllerMode(EPBPlayerControllerMode::Movement);

		// AttributeSet의 Movement 값을 PathDisplay에 전달
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (IsValid(ASC))
		{
			const float MovementRange = ASC->GetNumericAttribute(UPBTurnResourceAttributeSet::GetMovementAttribute());
			PC->SetPathDisplayMovementRange(MovementRange);
		}
	}

	// MoveCommand 이벤트를 수신
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
	// Movement 모드인 경우 None으로 복원
	if (ActorInfo)
	{
		APBGameplayPlayerController* PC = Cast<APBGameplayPlayerController>(ActorInfo->PlayerController.Get());
		if (IsValid(PC) && PC->GetControllerMode() == EPBPlayerControllerMode::Movement)
		{
			PC->SetControllerMode(EPBPlayerControllerMode::None);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPBGameplayAbility_Move::HandleMoveEvent(FGameplayEventData Payload)
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return;
	}

	APawn* MyPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!IsValid(MyPawn))
	{
		return;
	}

	// PC는 플레이어 전용 — AI인 경우 null
	APBGameplayPlayerController* PC = Cast<APBGameplayPlayerController>(ActorInfo->PlayerController.Get());

	// Payload에서 목표 위치 추출
	const UPBTargetPayload* TargetPayload = Cast<UPBTargetPayload>(Payload.OptionalObject);
	if (!IsValid(TargetPayload))
	{
		return;
	}

	const FVector HitLocation = TargetPayload->TargetData.TargetLocation;

	// NavPath 탐색
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!IsValid(NavSys))
	{
		return;
	}

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(
		GetWorld(), MyPawn->GetActorLocation(), HitLocation);

	if (!IsValid(NavPath) || !NavPath->IsValid())
	{
		return;
	}

	// AttributeSet에서 현재 남은 이동력 취득, AI는 무제한(-1)
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float MaxDist = IsValid(ASC)
		? ASC->GetNumericAttribute(UPBTurnResourceAttributeSet::GetMovementAttribute())
		: -1.0f;
	const FVector Destination = CalculateClampedDestination(NavPath->PathPoints, MaxDist);

	// 폰의 컨트롤러로 이동 명령 (플레이어/AI 공용)
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(MyPawn->GetController(), Destination);
}

FVector UPBGameplayAbility_Move::CalculateClampedDestination(const TArray<FVector>& PathPoints, float MaxDist) const
{
	if (PathPoints.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	if (MaxDist <= -1.0f)
	{
		return PathPoints.Last();
	}

	float AccumulatedDist = 0.0f;

	for (int32 i = 1; i < PathPoints.Num(); ++i)
	{
		const float SegDist = FVector::Dist(PathPoints[i - 1], PathPoints[i]);

		if (AccumulatedDist + SegDist >= MaxDist)
		{
			const float Remaining = MaxDist - AccumulatedDist;
			const float T = (SegDist > 0.0f) ? (Remaining / SegDist) : 0.0f;
			return FMath::Lerp(PathPoints[i - 1], PathPoints[i], T);
		}

		AccumulatedDist += SegDist;
	}

	return PathPoints.Last();
}
