// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInteractableComponent.h"
#include "PBInteractionAction.h"
#include "ProjectB3/Utils/PBGameplayStatics.h"
#include "Components/MeshComponent.h"
#include "ProjectB3/ProjectB3.h"

UPBInteractableComponent::UPBInteractableComponent()
{
}

void UPBInteractableComponent::OnFocus()
{
	if (bIsFocused)
	{
		return;
	}
	bIsFocused = true;

	TArray<UMeshComponent*> Meshes;
	UPBGameplayStatics::GetAllMeshComponents(GetOwner(), Meshes);

	SavedCustomDepthStates.Reset();
	for (UMeshComponent* Mesh : Meshes)
	{
		if (IsValid(Mesh))
		{
			// 원래 RenderCustomDepth 상태 저장
			SavedCustomDepthStates.Add(TObjectPtr<UMeshComponent>(Mesh), Mesh->bRenderCustomDepth);

			Mesh->SetRenderCustomDepth(true);
			Mesh->SetCustomDepthStencilValue(PBStencilValues::INTERACTION);
		}
	}
}

void UPBInteractableComponent::OnUnfocus()
{
	bIsFocused = false;

	TArray<UMeshComponent*> Meshes;
	UPBGameplayStatics::GetAllMeshComponents(GetOwner(), Meshes);

	for (UMeshComponent* Mesh : Meshes)
	{
		if (!IsValid(Mesh))
		{
			continue;
		}

		Mesh->SetCustomDepthStencilValue(0);

		// 원래 상태로 복원. 저장된 값이 없으면 false로 초기화
		const bool* bWasEnabled = SavedCustomDepthStates.Find(TObjectPtr<UMeshComponent>(Mesh));
		Mesh->SetRenderCustomDepth(bWasEnabled ? *bWasEnabled : false);
	}

	SavedCustomDepthStates.Reset();
}

void UPBInteractableComponent::OnInteract(AActor* Interactor)
{
	// 실행 가능한 행동 중 우선순위가 가장 높은 것을 선택
	UPBInteractionAction* BestAction = nullptr;
	int32 BestPriority = INT32_MIN;

	for (UPBInteractionAction* Action : InteractionActions)
	{
		if (!IsValid(Action))
		{
			continue;
		}

		if (!Action->CanInteract(Interactor))
		{
			continue;
		}

		const int32 ActionPriority = Action->GetPriority();
		if (ActionPriority > BestPriority)
		{
			BestPriority = ActionPriority;
			BestAction = Action;
		}
	}

	if (IsValid(BestAction))
	{
		BestAction->Execute(Interactor);
	}
}

bool UPBInteractableComponent::HasAvailableAction(AActor* Interactor) const
{
	for (UPBInteractionAction* Action : InteractionActions)
	{
		if (IsValid(Action) && Action->CanInteract(Interactor))
		{
			return true;
		}
	}
	return false;
}
