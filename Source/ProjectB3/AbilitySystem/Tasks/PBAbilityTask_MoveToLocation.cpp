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

	// SimpleMoveToLocation으로 이동 시작
	// 첫 호출 시 내부적으로 PathFollowingComponent를 생성하므로 컴포넌트 검색보다 먼저 호출해야 한다
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(Controller, Destination);

	// PathFollowingComponent 탐색: AIController이면 전용 접근자, 아니면 컴포넌트 검색
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

	// 이동 요청 ID 획득 및 완료 델리게이트 바인딩
	MoveRequestID = PFComp->GetCurrentRequestId();
	PFComp->OnRequestFinished.AddUObject(this, &UPBAbilityTask_MoveToLocation::HandleRequestFinished);

	// SimpleMoveToLocation이 동기적으로 완료된 경우 처리
	// (목적지가 이미 도달 범위 내이거나 유효 경로가 없으면 바인딩 전에 OnRequestFinished가 발생함)
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
