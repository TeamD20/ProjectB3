// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBPartyAIController.h"

#include "Navigation/CrowdFollowingComponent.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"

APBPartyAIController::APBPartyAIController(const FObjectInitializer& OI)
	: Super(OI.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
}

void APBPartyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	MoveState = EPBPartyMoveState::Idle;
}

void APBPartyAIController::OnUnPossess()
{
	StopFollowMove();
	Super::OnUnPossess();
}

void APBPartyAIController::MoveToTrailPoint(const FVector& TrailPoint)
{
	ExecuteMoveTo(TrailPoint, EPBPartyMoveState::MovingToTrail);
}

void APBPartyAIController::MoveToScatterPosition(const FVector& ScatterPos)
{
	ExecuteMoveTo(ScatterPos, EPBPartyMoveState::MovingToScatter);
}

void APBPartyAIController::StopFollowMove()
{
	StopMovement();
	MoveState = EPBPartyMoveState::Idle;
}

void APBPartyAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	// 관련 없는 이동 요청의 콜백은 무시
	if (RequestID != ActiveMoveRequestID)
	{
		return;
	}

	MoveState = EPBPartyMoveState::Idle;

	const bool bSuccess = Result.IsSuccess();
	OnPartyMoveCompleted.Broadcast(this, bSuccess);
}

void APBPartyAIController::ExecuteMoveTo(const FVector& Destination, EPBPartyMoveState NewState)
{
	// 이전 이동 중단
	StopMovement();

	MoveState = NewState;
	ActiveMoveRequestID = FAIRequestID::InvalidRequest;

	UWorld* World = GetWorld();
	if (!IsValid(World) || !IsValid(World->GetGameInstance()))
	{
		MoveState = EPBPartyMoveState::Idle;
		OnPartyMoveCompleted.Broadcast(this, false);
		return;
	}

	UPBEnvironmentSubsystem* EnvironmentSubsystem = World->GetGameInstance()->GetSubsystem<UPBEnvironmentSubsystem>();
	if (!IsValid(EnvironmentSubsystem))
	{
		MoveState = EPBPartyMoveState::Idle;
		OnPartyMoveCompleted.Broadcast(this, false);
		return;
	}

	if (EnvironmentSubsystem->RequestMoveToLocation(this, Destination, 50.f, false))
	{
		if (UPathFollowingComponent* PFC = GetPathFollowingComponent())
		{
			ActiveMoveRequestID = PFC->GetCurrentRequestId();

			if (!ActiveMoveRequestID.IsValid() || PFC->GetStatus() == EPathFollowingStatus::Idle)
			{
				MoveState = EPBPartyMoveState::Idle;
				OnPartyMoveCompleted.Broadcast(this, true);
			}
		}
		else
		{
			MoveState = EPBPartyMoveState::Idle;
			// 다음 틱으로 지연하여 콜스택 내 재귀 방지
			TWeakObjectPtr<APBPartyAIController> WeakThis(this);
			World->GetTimerManager().SetTimerForNextTick([WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->OnPartyMoveCompleted.Broadcast(WeakThis.Get(), false);
				}
			});
		}
	}
	else
	{
		// NavMesh 리빌드 등으로 이동 요청이 즉시 실패한 경우.
		// 동기 브로드캐스트 시 OnFollowerMoveCompleted -> MoveToScatterPosition -> ExecuteMoveTo 재귀로
		// 스택 오버플로우가 발생할 수 있으므로 다음 틱으로 지연한다.
		MoveState = EPBPartyMoveState::Idle;
		TWeakObjectPtr<APBPartyAIController> WeakThis(this);
		World->GetTimerManager().SetTimerForNextTick([WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OnPartyMoveCompleted.Broadcast(WeakThis.Get(), false);
			}
		});
	}
}
