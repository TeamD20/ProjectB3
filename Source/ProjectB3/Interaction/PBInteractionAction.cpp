// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInteractionAction.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

APawn* UPBInteractionAction::GetPawn(const AActor* Interactor) const
{
	// Interactor가 컨트롤러인 경우 빙의 폰 반환
	if (const AController* Controller = Cast<AController>(Interactor))
	{
		return Controller->GetPawn();
	}

	// Interactor가 폰인 경우 직접 반환
	return const_cast<APawn*>(Cast<APawn>(Interactor));
}

AController* UPBInteractionAction::GetController(const AActor* Interactor) const
{
	// Interactor가 컨트롤러인 경우 직접 반환
	if (AController* Controller = const_cast<AController*>(Cast<AController>(Interactor)))
	{
		return Controller;
	}

	// Interactor가 폰인 경우 컨트롤러 반환
	if (const APawn* Pawn = Cast<APawn>(Interactor))
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
}

int32 UPBInteractionAction::GetPriority_Implementation() const
{
	return Priority;
}
