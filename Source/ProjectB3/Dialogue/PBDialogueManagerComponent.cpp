// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDialogueManagerComponent.h"

#include "DialogueData.h"
#include "DialogueNode.h"
#include "DialogueSystemTypes.h"
#include "DialogueFeatures/DNodeFeature_Branch.h"
#include "GameFramework/PlayerController.h"
#include "ProjectB3/Camera/PBDialogueCameraActor.h"
#include "ProjectB3/Dialogue/Dice/PBDiceRollActor.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"
#include "ProjectB3/UI/Dialogue/PBDialogueWidget.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"
#include "ProjectB3/UI/PBUIManagerSubsystem.h"
#include "ProjectB3/UI/PBUITags.h"
#include "ProjectB3/UI/ViewModel/PBViewModelSubsystem.h"

void UPBDialogueManagerComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!IsValid(DiceRollActorClass))
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return;
    }

    DiceRollActor = World->SpawnActor<APBDiceRollActor>(DiceRollActorClass, DiceActorSpawnTransform);
}

void UPBDialogueManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(DiceRollActor))
    {
        DiceRollActor->Destroy();
        DiceRollActor = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

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

    // 대화 카메라 전부 파괴 후 Pawn 카메라로 원복
    DestroyAllDialogueCameras();
    PC->SetViewTargetWithBlend(PC->GetPawn(), 0.5f);

    if (UPBViewModelSubsystem* ViewModelSubsystem = UPBUIBlueprintLibrary::GetViewModelSubsystem(PC))
    {
        ViewModelSubsystem->SetVisibilityByTag(PBUITags::UI_ViewModel_SkillBar, true);
    }

    Super::OnDialogueEnd(CurrentNode);
}

APBDialogueCameraActor* UPBDialogueManagerComponent::GetOrCreateCamera(const FGameplayTag& InParticipantTag)
{
    // 이미 생성된 카메라가 있으면 반환
    if (TObjectPtr<APBDialogueCameraActor>* Found = ParticipantCameraMap.Find(InParticipantTag))
    {
        if (IsValid(*Found))
        {
            return *Found;
        }
    }

    UWorld* World = GetWorld();
    if (!IsValid(World))
    {
        return nullptr;
    }

    // 카메라 스폰 후 Map에 등록
    APBDialogueCameraActor* NewCamera = World->SpawnActor<APBDialogueCameraActor>();
    if (IsValid(NewCamera))
    {
        ParticipantCameraMap.Add(InParticipantTag, NewCamera);
    }

    return NewCamera;
}

void UPBDialogueManagerComponent::DestroyAllDialogueCameras()
{
    for (auto& Pair : ParticipantCameraMap)
    {
        if (IsValid(Pair.Value))
        {
            Pair.Value->Destroy();
        }
    }

    ParticipantCameraMap.Empty();
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

    // 분기 인덱스 저장 (0=성공, 1=실패)
    PendingDiceOptionId = bSuccess ? 0 : 1;

    // 3D 연출이 있으면 완료 후 결과 표시, 없으면 즉시 표시
    if (IsValid(DiceRollActor))
    {
        PendingDiceResult = Result;
        DiceRollActor->OnDiceRollFinished.AddDynamic(this, &ThisClass::OnDiceAnimationFinished);
        DiceRollActor->RollToNumber(Total);
    }
    else
    {
        DialogueViewModel->ShowDiceResult(Result);
    }
}

void UPBDialogueManagerComponent::ProgressDiceResult()
{
    ProgressDialogue(PendingDiceOptionId);
}

void UPBDialogueManagerComponent::OnDiceAnimationFinished(int32 ResultNumber)
{
    if (IsValid(DiceRollActor))
    {
        DiceRollActor->OnDiceRollFinished.RemoveDynamic(this, &ThisClass::OnDiceAnimationFinished);
    }

    if (IsValid(DialogueViewModel))
    {
        DialogueViewModel->ShowDiceResult(PendingDiceResult);
    }
}
