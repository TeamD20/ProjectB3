// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_DiceCheckBranch.h"

#include "DialogueNode.h"
#include "DialogueSystemTypes.h"
#include "DialogueFeatures/DNodeFeature_Branch.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "ProjectB3/UI/Dialogue/PBDialogueViewModel.h"

void UPBDNodeFeature_DiceCheckBranch::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    UPBDialogueViewModel* ViewModel = InDialogueContext.GetContextObject<UPBDialogueViewModel>();
    if (!IsValid(ViewModel))
    {
        return;
    }

    // ViewModel에 주사위 굴리기 정보 전달.
    // Manager는 RequestDiceRoll 시 ViewModel에서 DC를 읽으므로 별도 Manager 접근 불필요
    FPBDiceRollDisplayInfo Info;
    Info.DC = DC;
    Info.SkillName = FText::FromString(SkillCheckTag.GetTagName().ToString());

    ViewModel->ShowDiceRoll(Info);

    // bAutoRoll이면 즉시 굴리기
    if (bAutoRoll)
    {
        ViewModel->RequestDiceRoll();
    }
}

FName UPBDNodeFeature_DiceCheckBranch::GetNextNodeId(const int32 OptionId) const
{
    if (OptionId == 0)
    {
        return SuccessNodeId;
    }

    return FailureNodeId;
}

TArray<FDialogueNodeLink> UPBDNodeFeature_DiceCheckBranch::GetAllLinks() const
{
    TArray<FDialogueNodeLink> Links;

    FDialogueNodeLink SuccessLink;
    SuccessLink.SourceNodeId = GetDialogueNode()->NodeID;
    SuccessLink.LinkName = FName("Success");
    SuccessLink.TargetNodeId = SuccessNodeId;
    Links.Add(SuccessLink);

    FDialogueNodeLink FailureLink;
    FailureLink.SourceNodeId = GetDialogueNode()->NodeID;
    FailureLink.LinkName = FName("Failure");
    FailureLink.TargetNodeId = FailureNodeId;
    Links.Add(FailureLink);

    return Links;
}

void UPBDNodeFeature_DiceCheckBranch::UpdateLinks(const TArray<FDialogueNodeLink>& InLinks)
{
    for (const FDialogueNodeLink& Link : InLinks)
    {
        if (Link.LinkName == FName("Success"))
        {
            SuccessNodeId = Link.TargetNodeId;
        }
        else if (Link.LinkName == FName("Failure"))
        {
            FailureNodeId = Link.TargetNodeId;
        }
    }
}

void UPBDNodeFeature_DiceCheckBranch::GetDiceCheckInfo(FGameplayTag& OutSkillTag, int32& OutDC, bool& OutbAutoRoll) const
{
    OutSkillTag = SkillCheckTag;
    OutDC = DC;
    OutbAutoRoll = bAutoRoll;
}
