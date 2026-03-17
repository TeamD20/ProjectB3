// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInteractionAction.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"


AActor* UPBInteractionAction::GetOwner() const
{
	return GetTypedOuter<AActor>();
}

APawn* UPBInteractionAction::GetPawn(AActor* Interactor) const
{
	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		return Pawn;
	}
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller->GetPawn();
	}
	return nullptr;
}

AController* UPBInteractionAction::GetController(AActor* Interactor) const
{
	AController* Controller = Cast<AController>(Interactor);
	if (IsValid(Controller))
	{
		return Controller;
	}
	APawn* Pawn = Cast<APawn>(Interactor);
	if (IsValid(Pawn))
	{
		return Pawn->GetController();
	}
	return nullptr;
}

bool UPBInteractionAction::CanInteract_Implementation(AActor* Interactor) const
{
	// 기본 구현: 항상 실행 가능. 하위 클래스에서 조건을 재정의한다.
	return true;
}

void UPBInteractionAction::Execute_Implementation(AActor* Interactor)
{
	// 기본 구현: 없음. 하위 클래스에서 실제 행동을 구현한다.
	// 유지형인 경우 활성 상태로 전환
	if (IsSustained())
	{
		bIsActive = true;
	}
}

void UPBInteractionAction::EndInteraction_Implementation()
{
	// 기본 구현: 활성 상태 해제만 수행. 하위 클래스에서 정리 로직 추가
	bIsActive = false;
}

int32 UPBInteractionAction::GetPriority_Implementation() const
{
	return Priority;
}
