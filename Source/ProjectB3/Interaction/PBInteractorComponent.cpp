// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInteractorComponent.h"
#include "PBInteractableComponent.h"
#include "PBInteractionAction.h"
#include "PBInteractionInterface.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UPBInteractorComponent::UPBInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false);
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

void UPBInteractorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 활성 상호작용이 무효화되었으면 정리
	if (!ActiveAction.IsValid() || !ActiveInteractable.IsValid())
	{
		EndActiveInteraction();
		return;
	}

	// 거리 유지가 필요한 경우 범위 체크
	if (ActiveAction->bRequiresRange)
	{
		AActor* TargetActor = ActiveInteractable->GetOwner();
		if (!IsWithinRange(TargetActor))
		{
			EndActiveInteraction();
		}
	}
}

void UPBInteractorComponent::Interact()
{
	if (!IsValid(FocusedComponent))
	{
		return;
	}

	// 이미 유지형 상호작용 중이면 먼저 종료
	if (ActiveAction.IsValid())
	{
		EndActiveInteraction();
	}

	FocusedComponent->OnInteract(GetOwner());

	// OnInteract 후 유지형 Action이 시작되었으면 추적 시작
	UPBInteractionAction* NewActiveAction = FocusedComponent->GetActiveAction();
	if (IsValid(NewActiveAction) && NewActiveAction->IsSustained() && NewActiveAction->IsActive())
	{
		SetActiveInteraction(NewActiveAction, FocusedComponent);
	}
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

void UPBInteractorComponent::EndActiveInteraction()
{
	if (ActiveInteractable.IsValid())
	{
		ActiveInteractable->EndActiveInteraction();
	}

	ActiveAction.Reset();
	ActiveInteractable.Reset();
	SetComponentTickEnabled(false);
}

void UPBInteractorComponent::SetActiveInteraction(UPBInteractionAction* Action, UPBInteractableComponent* Interactable)
{
	ActiveAction = Action;
	ActiveInteractable = Interactable;

	// 거리 유지가 필요한 경우에만 Tick 활성화
	if (IsValid(Action) && Action->bRequiresRange)
	{
		SetComponentTickEnabled(true);
	}
}
