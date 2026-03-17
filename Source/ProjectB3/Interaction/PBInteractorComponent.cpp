// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInteractorComponent.h"
#include "PBInteractableComponent.h"
#include "PBInteractionInterface.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UPBInteractorComponent::UPBInteractorComponent()
{
}

void UPBInteractorComponent::TryFocus(AActor* Actor)
{
	// 무시 목록에 포함된 액터는 포커스 해제
	for (const TWeakObjectPtr<AActor>& IgnoredActor : IgnoreActors)
	{
		if (IgnoredActor.Get() == Actor)
		{
			ClearFocus();
			return;
		}
	}

	// 인터페이스 미구현 또는 범위 초과 시 포커스 해제
	if (!Actor || !Actor->Implements<UPBInteractionInterface>() || !IsWithinRange(Actor))
	{
		ClearFocus();
		return;
	}

	IPBInteractionInterface* Interaction = Cast<IPBInteractionInterface>(Actor);
	SetFocus(Interaction ? Interaction->GetInteractableComponent() : nullptr);
}

void UPBInteractorComponent::SetIgnoreActors(const TArray<AActor*>& Actors)
{
	IgnoreActors.Reset();
	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			IgnoreActors.Add(TWeakObjectPtr<AActor>(Actor));
		}
	}
}

void UPBInteractorComponent::SetFocus(UPBInteractableComponent* NewFocus)
{
	// 동일 대상이면 무시
	if (FocusedComponent == NewFocus)
	{
		return;
	}

	// 기존 포커스 해제
	if (IsValid(FocusedComponent))
	{
		FocusedComponent->OnUnfocus();
	}

	FocusedComponent = NewFocus;

	// 신규 포커스 진입
	if (IsValid(FocusedComponent))
	{
		FocusedComponent->OnFocus();
	}
}

void UPBInteractorComponent::ClearFocus()
{
	SetFocus(nullptr);
}

void UPBInteractorComponent::Interact()
{
	if (!IsValid(FocusedComponent))
	{
		return;
	}

	FocusedComponent->OnInteract(GetOwner());
}

bool UPBInteractorComponent::HasFocus() const
{
	return IsValid(FocusedComponent);
}

bool UPBInteractorComponent::IsWithinRange(const AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	// 컨트롤러에 부착된 컴포넌트이므로 빙의 폰의 위치를 기준으로 거리 계산
	const AController* Controller = Cast<AController>(GetOwner());
	const APawn* Pawn = IsValid(Controller) ? Controller->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return false;
	}

	const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), Target->GetActorLocation());
	return DistSq <= FMath::Square(MaxInteractionDistance);
}
