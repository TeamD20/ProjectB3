// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBGameplayStatics.h"

#include "DialogueData.h"
#include "DialogueSystemTypes.h"
#include "Components/MeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Dialogue/PBDialogueManagerComponent.h"
#include "ProjectB3/Environment/PBEnvironmentSubsystem.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBGameplayStatics::GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes)
{
	if (!IsValid(Actor))
	{
		return;
	}

	// 현재 액터의 모든 MeshComponent 수집
	Actor->GetComponents<UMeshComponent>(OutMeshes, /*bIncludeFromChildActors=*/false);

	// ChildActorComponent를 통한 자식 액터 재귀 탐색
	TArray<UChildActorComponent*> ChildActorComps;
	Actor->GetComponents<UChildActorComponent>(ChildActorComps);
	for (UChildActorComponent* ChildActorComp : ChildActorComps)
	{
		if (!IsValid(ChildActorComp))
		{
			continue;
		}

		TArray<UMeshComponent*> ChildMeshes;
		GetAllMeshComponents(ChildActorComp->GetChildActor(), ChildMeshes);
		OutMeshes.Append(ChildMeshes);
	}

	// 부착된 액터 재귀 탐색
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors, /*bResetArray=*/true, /*bRecursivelyIncludeAttachedActors=*/false);
	for (AActor* AttachedActor : AttachedActors)
	{
		TArray<UMeshComponent*> AttachedMeshes;
		GetAllMeshComponents(AttachedActor, AttachedMeshes);
		OutMeshes.Append(AttachedMeshes);
	}
}

bool UPBGameplayStatics::SimpleMoveToLocation(AController* Controller, const FVector& GoalLocation, float AcceptanceRadius)
{
	if (!IsValid(Controller))
	{
		return false;
	}

	UWorld* World = Controller->GetWorld();
	if (!IsValid(World) || !IsValid(World->GetGameInstance()))
	{
		return false;
	}

	if (UPBEnvironmentSubsystem* EnvironmentSubsystem = World->GetGameInstance()->GetSubsystem<UPBEnvironmentSubsystem>())
	{
		return EnvironmentSubsystem->RequestMoveToLocation(Controller, GoalLocation, AcceptanceRadius, false);
	}

	return false;
}

void UPBGameplayStatics::StartDialogue(APawn* Interactor, AActor* TargetActor, UDialogueData* DialogueData)
{
	if (!IsValid(Interactor) || !IsValid(DialogueData))
	{
		return;
	}
	
	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (!IsValid(PC))
	{
		return;
	}
	
	UPBDialogueManagerComponent* DialogueManager = PC->FindComponentByClass<UPBDialogueManagerComponent>();
	if (!IsValid(DialogueManager))
	{
		return;
	}
	
	// Context 구성
	FDialogueSystemContext Context;
	Context.InstigatorController = PC;
	Context.InstigatorActor = Interactor;
	Context.TargetActor = TargetActor;
	Context.ContextObject = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBDialogueViewModel>(PC);

	// NpcParticipantTag가 설정되어 있으면 ParticipantActors 맵에 NPC Actor 등록
	if (APBCharacterBase* PBCharacter = Cast<APBCharacterBase>(TargetActor))
	{
		const FGameplayTag IdentityTag = PBCharacter->GetCharacterIdentity().IdentityTag;
		if (IdentityTag.IsValid())
		{
			Context.RegisterParticipantActor(IdentityTag, TargetActor);
		}
	}

	// PC가 현재 조종 중인 Pawn을 Player 태그로 등록
	if (IsValid(Interactor))
	{
		Context.RegisterParticipantActor(PBGameplayTags::Player, Interactor);
	}

	// 주인공 파티 전원을 CharacterIdentityTag 기준으로 Participant 등록
	APBGameplayPlayerState* PS = PC->GetPlayerState<APBGameplayPlayerState>();
	if (IsValid(PS))
	{
		for (AActor* Member : PS->GetPartyMembers())
		{
			APBCharacterBase* Character = Cast<APBCharacterBase>(Member);
			if (!IsValid(Character))
			{
				continue;
			}

			const FGameplayTag IdentityTag = Character->GetCharacterIdentity().IdentityTag;
			if (IdentityTag.IsValid())
			{
				Context.RegisterParticipantActor(IdentityTag, Character);
			}
		}
	}
	
	// 대화 시작
	DialogueManager->StartDialogue(DialogueData, Context);
}
