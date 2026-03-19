// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_StartCombatByFaction.h"

#include "DialogueSystemTypes.h"
#include "EngineUtils.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/Combat/IPBCombatParticipant.h"
#include "ProjectB3/Combat/PBCombatManagerSubsystem.h"
#include "ProjectB3/Game/PBGameplayGameMode.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"

void UPBDNodeFeature_StartCombatByFaction::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    AActor* TargetActor = InDialogueContext.TargetActor.Get();
    if (!IsValid(TargetActor))
    {
        return;
    }

    UWorld* World = TargetActor->GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    TArray<AActor*> Combatants;
    CollectNearbySameFactionCombatants(TargetActor, Combatants);
    CollectPlayerPartyMembers(InDialogueContext, Combatants);

    if (Combatants.Num() < 2)
    {
        return;
    }

    if (APBGameplayGameMode* GameMode = World->GetAuthGameMode<APBGameplayGameMode>())
    {
        GameMode->InitiateCombat(Combatants);
        return;
    }

    if (UPBCombatManagerSubsystem* CombatManager = World->GetSubsystem<UPBCombatManagerSubsystem>())
    {
        CombatManager->StartCombat(Combatants);
    }
}

void UPBDNodeFeature_StartCombatByFaction::CollectNearbySameFactionCombatants(AActor* InCenterActor, TArray<AActor*>& OutCombatants) const
{
    if (!IsValid(InCenterActor))
    {
        return;
    }

    APBCharacterBase* CenterCharacter = Cast<APBCharacterBase>(InCenterActor);
    if (!IsValid(CenterCharacter))
    {
        return;
    }

    UWorld* World = CenterCharacter->GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    const FVector CenterLocation = CenterCharacter->GetActorLocation();
    const FGameplayTag CenterFactionTag = CenterCharacter->GetFactionTag();

    if (!CenterFactionTag.IsValid())
    {
        return;
    }

    for (TActorIterator<APBCharacterBase> It(World); It; ++It)
    {
        APBCharacterBase* Candidate = *It;
        if (!IsValid(Candidate))
        {
            continue;
        }

        if (!bIncludeTargetActor && Candidate == CenterCharacter)
        {
            continue;
        }

        const float DistSquared = FVector::DistSquared(CenterLocation, Candidate->GetActorLocation());
        if (DistSquared > FMath::Square(SearchRadius))
        {
            continue;
        }

        if (!IsFactionMatched(CenterFactionTag, Candidate->GetFactionTag()))
        {
            continue;
        }

        AddUniqueCombatant(Candidate, OutCombatants);
    }
}

void UPBDNodeFeature_StartCombatByFaction::CollectPlayerPartyMembers(const FDialogueSystemContext& InDialogueContext, TArray<AActor*>& OutCombatants) const
{
    APlayerController* InstigatorPC = InDialogueContext.InstigatorController.Get();
    if (!IsValid(InstigatorPC))
    {
        return;
    }

    APBGameplayPlayerState* PlayerState = InstigatorPC->GetPlayerState<APBGameplayPlayerState>();
    if (!IsValid(PlayerState))
    {
        return;
    }

    const TArray<AActor*> PartyMembers = PlayerState->GetPartyMembers();
    for (AActor* PartyMember : PartyMembers)
    {
        AddUniqueCombatant(PartyMember, OutCombatants);
    }
}

void UPBDNodeFeature_StartCombatByFaction::AddUniqueCombatant(AActor* InActor, TArray<AActor*>& OutCombatants) const
{
    if (!IsValid(InActor))
    {
        return;
    }

    if (!Cast<IPBCombatParticipant>(InActor))
    {
        return;
    }

    OutCombatants.AddUnique(InActor);
}

bool UPBDNodeFeature_StartCombatByFaction::IsFactionMatched(const FGameplayTag& InLhs, const FGameplayTag& InRhs) const
{
    if (!InLhs.IsValid() || !InRhs.IsValid())
    {
        return false;
    }

    if (bMatchFactionExactly)
    {
        return InLhs.MatchesTagExact(InRhs);
    }

    return InLhs.MatchesTag(InRhs) || InRhs.MatchesTag(InLhs);
}
