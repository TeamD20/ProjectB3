// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInteraction_DialogueAction.h"

#include "DialogueData.h"
#include "DialogueSystemTypes.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Dialogue/PBDialogueManagerComponent.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/Interaction/PBInteractorComponent.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

UPBInteraction_DialogueAction::UPBInteraction_DialogueAction()
{
    InteractionType = EPBInteractionType::Sustained;
    bRequiresRange = true;
}

bool UPBInteraction_DialogueAction::CanInteract_Implementation(AActor* Interactor) const
{
    if (!IsValid(DialogueDataAsset))
    {
        return false;
    }
    
    if (APBCharacterBase* PBCharacter = Cast<APBCharacterBase>(GetOwner()))
    {
        if (PBCharacter->IsDead() || PBCharacter->IsIncapacitated())
        {
            return false;
        }
    }

    // Interactor의 DialogueManagerComponent 획득
    APlayerController* PC = Cast<APlayerController>(GetController(Interactor));
    if (!IsValid(PC))
    {
        return false;
    }

    UPBDialogueManagerComponent* DialogueManager = PC->FindComponentByClass<UPBDialogueManagerComponent>();
    if (!IsValid(DialogueManager))
    {
        return false;
    }

    // 이미 대화 중이면 불가
    if (IsValid(DialogueManager->GetCurrentDialogueData()))
    {
        return false;
    }

    return true;
}

void UPBInteraction_DialogueAction::Execute_Implementation(AActor* Interactor)
{
    APlayerController* PC = Cast<APlayerController>(GetController(Interactor));
    if (!IsValid(PC))
    {
        return;
    }

    UPBDialogueManagerComponent* DialogueManager = PC->FindComponentByClass<UPBDialogueManagerComponent>();
    if (!IsValid(DialogueManager))
    {
        return;
    }

    // InteractorComponent 캐시
    APawn* Pawn = GetPawn(Interactor);
    CachedInteractorComponent = PC->FindComponentByClass<UPBInteractorComponent>();
    CachedDialogueManager = DialogueManager;

    // Context 구성
    FDialogueSystemContext Context;
    Context.InstigatorController = PC;
    Context.InstigatorActor = Pawn;
    Context.TargetActor = GetOwner();
    Context.ContextObject = UPBUIBlueprintLibrary::GetOrCreateGlobalViewModel<UPBDialogueViewModel>(PC);

    // NpcParticipantTag가 설정되어 있으면 ParticipantActors 맵에 NPC Actor 등록
    if (APBCharacterBase* PBCharacter = Cast<APBCharacterBase>(GetOwner()))
    {
        const FGameplayTag IdentityTag = PBCharacter->GetCharacterIdentity().IdentityTag;
        if (IdentityTag.IsValid())
        {
            Context.RegisterParticipantActor(IdentityTag, GetOwner());
        }
    }

    // PC가 현재 조종 중인 Pawn을 Player 태그로 등록
    if (IsValid(Pawn))
    {
        Context.RegisterParticipantActor(PBGameplayTags::Player, Pawn);
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

    // 대화 종료 델리게이트 바인딩
    DialogueManager->OnDialogueEndDelegate.AddDynamic(this, &ThisClass::HandleDialogueEnded);

    // 대화 시작
    DialogueManager->StartDialogue(DialogueDataAsset, Context);

    Super::Execute_Implementation(Interactor);
}

void UPBInteraction_DialogueAction::EndInteraction_Implementation()
{
    if (CachedDialogueManager.IsValid())
    {
        // 대화 강제 종료: EndNode가 없으면 ProgressDialogue로 종료 처리
        CachedDialogueManager->OnDialogueEndDelegate.RemoveDynamic(this, &ThisClass::HandleDialogueEnded);
        CachedDialogueManager.Reset();
    }

    CachedInteractorComponent.Reset();
    Super::EndInteraction_Implementation();
}

void UPBInteraction_DialogueAction::HandleDialogueEnded(FDialogueChangeMessage DialogueChangeMessage)
{
    if (CachedDialogueManager.IsValid())
    {
        CachedDialogueManager->OnDialogueEndDelegate.RemoveDynamic(this, &ThisClass::HandleDialogueEnded);
        CachedDialogueManager.Reset();
    }

    if (CachedInteractorComponent.IsValid())
    {
        CachedInteractorComponent->EndActiveInteraction();
        CachedInteractorComponent.Reset();
    }
}
