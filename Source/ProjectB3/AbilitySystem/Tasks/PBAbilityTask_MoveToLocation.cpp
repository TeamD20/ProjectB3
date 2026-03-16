// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilityTask_MoveToLocation.h"
#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "GameFramework/Pawn.h"

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

	// AI: AAIController::MoveTo로 직접 이동 요청
	// SimpleMoveToLocation 대비 장점:
	//   1. FPathFollowingRequestResult로 RequestID 직접 획득 (캡처 타이밍 문제 해소)
	//   2. AcceptanceRadius 명시 제어 (NavMesh 투영 오차로 인한 미도착 방지)
	//   3. AlreadyAtGoal 명시 처리
	if (AAIController* AICon = Cast<AAIController>(Controller))
	{
		UPathFollowingComponent* PFComp = AICon->GetPathFollowingComponent();
		if (!IsValid(PFComp))
		{
			OnMoveCompleted.Broadcast(EPathFollowingResult::Invalid);
			EndTask();
			return;
		}

		FAIMoveRequest MoveReq;
		MoveReq.SetGoalLocation(Destination);
		MoveReq.SetAcceptanceRadius(50.f);
		MoveReq.SetUsePathfinding(true);

		const FPathFollowingRequestResult Result = AICon->MoveTo(MoveReq);

		if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
		{
			MoveRequestID = Result.MoveId;
			WeakPathFollowingComp = PFComp;
			PFComp->OnRequestFinished.AddUObject(
				this, &UPBAbilityTask_MoveToLocation::HandleRequestFinished);
		}
		else if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
		{
			OnMoveCompleted.Broadcast(EPathFollowingResult::Success);
			EndTask();
		}
		else
		{
			OnMoveCompleted.Broadcast(EPathFollowingResult::Invalid);
			EndTask();
		}
		return;
	}

	// Player: SimpleMoveToLocation 폴백 (PlayerController용)
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(Controller, Destination);

	UPathFollowingComponent* PFComp = Controller->FindComponentByClass<UPathFollowingComponent>();
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
