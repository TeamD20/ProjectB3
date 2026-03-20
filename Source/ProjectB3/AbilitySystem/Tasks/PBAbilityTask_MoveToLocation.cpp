// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilityTask_MoveToLocation.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "ProjectB3/Utils/PBGameplayStatics.h"

UPBAbilityTask_MoveToLocation* UPBAbilityTask_MoveToLocation::CreateTask(
	UGameplayAbility* OwningAbility, FVector InDestination)
{
	UPBAbilityTask_MoveToLocation* Task = NewAbilityTask<UPBAbilityTask_MoveToLocation>(OwningAbility);
	Task->Destination = InDestination;
	return Task;
}

void UPBAbilityTask_MoveToLocation::Activate()
{
	APawn* Pawn = Cast<APawn>(GetAvatarActor());
	if (!IsValid(Pawn))
	{
		OnMoveCompleted.Broadcast(EPathFollowingResult::Invalid);
		EndTask();
		return;
	}

	AController* Controller = Pawn->GetController();
	if (!IsValid(Controller))
	{
		OnMoveCompleted.Broadcast(EPathFollowingResult::Invalid);
		EndTask();
		return;
	}

	if (!UPBGameplayStatics::SimpleMoveToLocation(Controller, Destination, 50.f))
	{
		OnMoveCompleted.Broadcast(EPathFollowingResult::Invalid);
		EndTask();
		return;
	}

	UPathFollowingComponent* PFComp = nullptr;
	if (AAIController* AICon = Cast<AAIController>(Controller))
	{
		PFComp = AICon->GetPathFollowingComponent();
	}
	else
	{
		PFComp = Controller->FindComponentByClass<UPathFollowingComponent>();
	}

	if (!IsValid(PFComp))
	{
		OnMoveCompleted.Broadcast(EPathFollowingResult::Invalid);
		EndTask();
		return;
	}

	WeakPathFollowingComp = PFComp;
	MoveRequestID = PFComp->GetCurrentRequestId();
	PFComp->OnRequestFinished.AddUObject(
		this, &UPBAbilityTask_MoveToLocation::HandleRequestFinished);

	if (!MoveRequestID.IsValid() || PFComp->GetStatus() == EPathFollowingStatus::Idle)
	{
		OnMoveCompleted.Broadcast(EPathFollowingResult::Success);
		EndTask();
	}
}

void UPBAbilityTask_MoveToLocation::OnDestroy(bool bInOwnerFinished)
{
	UPathFollowingComponent* PFComp = WeakPathFollowingComp.Get();
	if (IsValid(PFComp))
	{
		PFComp->OnRequestFinished.RemoveAll(this);

		// 아직 이동 중이면 중단
		if (MoveRequestID.IsValid() && PFComp->GetStatus() != EPathFollowingStatus::Idle)
		{
			PFComp->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, MoveRequestID);
		}
	}

	Super::OnDestroy(bInOwnerFinished);
}

void UPBAbilityTask_MoveToLocation::HandleRequestFinished(
	FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (RequestID != MoveRequestID)
	{
		return;
	}

	MoveRequestID = FAIRequestID::InvalidRequest;
	OnMoveCompleted.Broadcast(Result.Code);
	EndTask();
}
