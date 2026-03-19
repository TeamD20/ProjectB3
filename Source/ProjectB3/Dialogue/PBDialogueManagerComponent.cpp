// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDialogueManagerComponent.h"

#include "DialogueData.h"
#include "DialogueNode.h"
#include "DialogueSystemTypes.h"
#include "DialogueFeatures/DNodeFeature_Branch.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"
#include "ProjectB3/UI/Dialogue/PBDialogueWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/PBUIManagerSubsystem.h"
#include "ProjectB3/UI/PBUITags.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"

void UPBDialogueManagerComponent::PreStartDialogue(UDialogueData* InDialogueData, const FDialogueSystemContext& InContext)
{
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!IsValid(PC))
    {
        return;
    }

    // Feature.OnStartDialogueNode보다 먼저 ViewModel을 생성하여 Context에 등록
    CreateAndInitViewModel(PC);
}

void UPBDialogueManagerComponent::CreateAndInitViewModel(APlayerController* PC)
{
    UPBViewModelSubsystem* VMSubsystem = UPBUIBlueprintLibrary::GetViewModelSubsystem(PC);
    if (!IsValid(VMSubsystem))
    {
        return;
    }

    DialogueViewModel = VMSubsystem->GetOrCreateGlobalViewModel<UPBDialogueViewModel>();
    if (!IsValid(DialogueViewModel))
    {
        return;
    }

    DialogueViewModel->InitializeForPlayer(PC->GetLocalPlayer());
    DialogueViewModel->SetDialogueManager(this);
    DialogueViewModel->SetDesiredVisibility(true);
}

void UPBDialogueManagerComponent::OnDialogueStart(UDialogueNode* CurrentNode)
{
    Super::OnDialogueStart(CurrentNode);
    
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!IsValid(PC))
    {
        return;
    }
    
    if (!IsValid(DialogueWidgetClass))
    {
        return;
    }

    // Widget Push (ViewModel은 StartDialogue에서 이미 생성됨)
    UPBWidgetBase* Widget = UPBUIBlueprintLibrary::PushUI(PC, DialogueWidgetClass);
    CachedDialogueWidget = Cast<UPBDialogueWidget>(Widget);
    
    if (UPBViewModelSubsystem* ViewModelSubsystem = UPBUIBlueprintLibrary::GetViewModelSubsystem(PC))
    {
        ViewModelSubsystem->SetVisibilityByTag(PBUITags::UI_ViewModel_SkillBar, false);
    }
}

void UPBDialogueManagerComponent::OnDialogueChanged(UDialogueNode* CurrentNode)
{
    Super::OnDialogueChanged(CurrentNode);
    
    if (!IsValid(DialogueViewModel) || !IsValid(CurrentNode))
    {
        return;
    }

    // 화자 정보를 구성하여 ViewModel에 전달
    FPBDialogueParticipantDisplayInfo SpeakerInfo = BuildSpeakerInfo(CurrentNode);
    DialogueViewModel->SetSpeakerInfo(SpeakerInfo);
}

void UPBDialogueManagerComponent::OnDialogueEnd(UDialogueNode* CurrentNode)
{
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!IsValid(PC))
    {
        return;
    }

    // Widget Pop
    if (CachedDialogueWidget.IsValid())
    {
        UPBUIBlueprintLibrary::PopUI(PC, CachedDialogueWidget.Get());
        CachedDialogueWidget.Reset();
    }

    // ViewModel 정리
    if (IsValid(DialogueViewModel))
    {
        DialogueViewModel->SetDesiredVisibility(false);
        DialogueViewModel->Deinitialize();

        UPBViewModelSubsystem* VMSubsystem = UPBUIBlueprintLibrary::GetViewModelSubsystem(PC);
        if (IsValid(VMSubsystem))
        {
            VMSubsystem->UnregisterGlobalViewModel(UPBDialogueViewModel::StaticClass());
        }

        DialogueViewModel = nullptr;
    }
    
    if (UPBViewModelSubsystem* ViewModelSubsystem = UPBUIBlueprintLibrary::GetViewModelSubsystem(PC))
    {
        ViewModelSubsystem->SetVisibilityByTag(PBUITags::UI_ViewModel_SkillBar, true);
    }
    
    Super::OnDialogueEnd(CurrentNode);
}

FPBDialogueParticipantDisplayInfo UPBDialogueManagerComponent::BuildSpeakerInfo(UDialogueNode* Node) const
{
    FPBDialogueParticipantDisplayInfo Info;
    if (!IsValid(Node))
    {
        return Info;
    }

    Info.ParticipantTag = Node->ParticipantTag;

    // DialogueData의 Participants에서 색상 정보 조회
    if (IsValid(GetCurrentDialogueData()))
    {
        FDialogueParticipantInfo ParticipantInfo;
        if (GetCurrentDialogueData()->DialogueParticipants.TryGetParticipantInfo(Node->ParticipantTag, ParticipantInfo))
        {
            Info.ParticipantName = FText::FromName(ParticipantInfo.ParticipantID);
            Info.ParticipantColor = FLinearColor(ParticipantInfo.ParticipantColor);
        }
    }

    return Info;
}

void UPBDialogueManagerComponent::RequestDiceRoll()
{
    if (!IsValid(DialogueViewModel))
    {
        return;
    }

    // ViewModel에 캐시된 주사위 정보(DiceCheckBranch가 ShowDiceRoll로 설정)에서 DC 읽기
    const FPBDiceRollDisplayInfo& PendingInfo = DialogueViewModel->GetDiceRollInfo();
    const int32 DC = PendingInfo.DC;

    // TODO: GAS 연동으로 캐릭터 ASC에서 스킬 수정치 조회
    const int32 Roll = FMath::RandRange(1, 20);
    const int32 Modifier = 0;
    const int32 Total = Roll + Modifier;
    const bool bSuccess = Total >= DC;

    FPBDiceRollDisplayInfo Result = PendingInfo;
    Result.Modifier = Modifier;
    Result.RollResult = Roll;
    Result.TotalResult = Total;
    Result.bSuccess = bSuccess;
    Result.bNatural20 = (Roll == 20);

    // 현재 노드의 Branch에 결과 전달 (0=성공, 1=실패)
    if (UDialogueNode* Node = GetCurrentDialogueNode())
    {
        if (UDNodeFeature_Branch* Branch = Node->FindBranch())
        {
            Branch->SelectBranchIndex(bSuccess ? 0 : 1);
        }
    }

    DialogueViewModel->ShowDiceResult(Result);
}
