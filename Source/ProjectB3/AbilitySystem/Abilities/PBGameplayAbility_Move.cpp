// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility_Move.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "ProjectB3/AbilitySystem/Tasks/PBAbilityTask_MoveToLocation.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/Player/PBGameplayPlayerController.h"
#include "ProjectB3/AbilitySystem/Payload/PBTargetPayload.h"
#include "ProjectB3/AbilitySystem/Attributes/PBTurnResourceAttributeSet.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/Combat/PBCombatSystemLibrary.h"

UPBGameplayAbility_Move::UPBGameplayAbility_Move()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(PBGameplayTags::Ability_Active_Move);
	SetAssetTags(AssetTags);
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
		if (IsValid(AvatarActor) && IsValid(ASC) && MovePathPoints.Num() > 0)
		{
			const float ActualDistance = CalculateDistanceAlongPath(AvatarActor->GetActorLocation());
			if (ActualDistance > 0.0f)
			{
				ASC->ApplyModToAttribute(UPBTurnResourceAttributeSet::GetMovementAttribute(), EGameplayModOp::Additive, -ActualDistance);
			}
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
				PC->SetControllerMode(EPBPlayerControllerMode::None);
			}
			else if (PC->GetControllerMode() == EPBPlayerControllerMode::TurnMovement)
			{
				PC->SetControllerMode(EPBPlayerControllerMode::None);
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
		return;
	}

	APawn* MyPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!IsValid(MyPawn))
	{
		return;
	}

	// PC는 플레이어 전용 — AI인 경우 null
	APBGameplayPlayerController* PC = GetPBPlayerController();

	// Payload에서 목표 위치 추출
	const UPBTargetPayload* TargetPayload = Cast<UPBTargetPayload>(Payload.OptionalObject);
	if (!IsValid(TargetPayload))
	{
		return;
	}

	const FVector HitLocation = TargetPayload->TargetData.GetSingleTargetLocation();

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
	MovePathPoints.Reset();
	const FVector Destination = CalculateClampedDestination(NavPath->PathPoints, MaxDist, MovePathPoints);

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

FVector UPBGameplayAbility_Move::CalculateClampedDestination(
	const TArray<FVector>& PathPoints, float MaxDist, TArray<FVector>& OutClampedPath) const
{
	if (PathPoints.Num() == 0)
	{
		return FVector::ZeroVector;
	}

	// 무제한 이동력: 전체 경로 반환
	if (MaxDist <= -1.0f)
	{
		OutClampedPath = PathPoints;
		return PathPoints.Last();
	}

	OutClampedPath.Add(PathPoints[0]);
	float AccumulatedDist = 0.0f;

	for (int32 i = 1; i < PathPoints.Num(); ++i)
	{
		const float SegDist = FVector::Dist(PathPoints[i - 1], PathPoints[i]);

		if (AccumulatedDist + SegDist >= MaxDist)
		{
			const float Remaining = MaxDist - AccumulatedDist;
			const float T = (SegDist > 0.0f) ? (Remaining / SegDist) : 0.0f;
			const FVector ClampedPoint = FMath::Lerp(PathPoints[i - 1], PathPoints[i], T);
			OutClampedPath.Add(ClampedPoint);
			return ClampedPoint;
		}

		OutClampedPath.Add(PathPoints[i]);
		AccumulatedDist += SegDist;
	}

	return PathPoints.Last();
}

float UPBGameplayAbility_Move::CalculateDistanceAlongPath(const FVector& CurrentLocation) const
{
	// MovePathPoints의 각 세그먼트에 현재 위치를 투영(projection)하여
	// 가장 가까운 세그먼트를 찾고, 시작점부터 해당 투영 지점까지의 경로 누적 거리를 반환한다.
	// 반응행동 등으로 이동이 중단된 경우에도 실제 이동한 경로 거리에 가깝도록 구한다.
	if (MovePathPoints.Num() < 2)
	{
		return 0.0f;
	}

	// 1. 현재 위치에 가장 가까운 경로 세그먼트 탐색
	float NearestDistSq = MAX_FLT;
	int32 NearestSegIndex = 0;
	float NearestSegRatio = 0.0f; // 세그먼트 내 투영 비율 (0.0 = 시작, 1.0 = 끝)

	for (int32 i = 0; i < MovePathPoints.Num() - 1; ++i)
	{
		const FVector SegStart = MovePathPoints[i];
		const FVector SegEnd = MovePathPoints[i + 1];
		const FVector ProjectedPoint = FMath::ClosestPointOnSegment(CurrentLocation, SegStart, SegEnd);
		const float DistSqToProjection = FVector::DistSquared(CurrentLocation, ProjectedPoint);

		if (DistSqToProjection < NearestDistSq)
		{
			NearestDistSq = DistSqToProjection;
			NearestSegIndex = i;

			const float SegLength = FVector::Dist(SegStart, SegEnd);
			NearestSegRatio = (SegLength > 0.0f)
				? FVector::Dist(SegStart, ProjectedPoint) / SegLength
				: 0.0f;
		}
	}

	// 2. 시작점 ~ 투영 지점까지의 경로 누적 거리 계산
	// 완전히 통과한 세그먼트들의 길이 합산
	float TotalTraveledDist = 0.0f;
	for (int32 i = 0; i < NearestSegIndex; ++i)
	{
		TotalTraveledDist += FVector::Dist(MovePathPoints[i], MovePathPoints[i + 1]);
	}
	// 현재 세그먼트의 부분 거리 추가
	TotalTraveledDist += FVector::Dist(MovePathPoints[NearestSegIndex], MovePathPoints[NearestSegIndex + 1]) * NearestSegRatio;

	return TotalTraveledDist;
}
