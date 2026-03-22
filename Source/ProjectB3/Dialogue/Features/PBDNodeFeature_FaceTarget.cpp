// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDNodeFeature_FaceTarget.h"

#include "DialogueSystemTypes.h"

void UPBDNodeFeature_FaceTarget::OnStartDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    AActor* SubjectActor = FindParticipantActor(InDialogueContext, SubjectParticipantTag);
    AActor* TargetActor = FindParticipantActor(InDialogueContext, TargetParticipantTag);

    if (!IsValid(SubjectActor) || !IsValid(TargetActor) || SubjectActor == TargetActor)
    {
        return;
    }

    // 진행 중인 회전이 있으면 먼저 정리
    if (RotationTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
    }

    SubjectActorRef = SubjectActor;
    StartYaw = SubjectActor->GetActorRotation().Yaw;

    // 수평면 기준 바라볼 방향의 Yaw 계산
    const FVector LookDir = (TargetActor->GetActorLocation() - SubjectActor->GetActorLocation()).GetSafeNormal2D();
    TargetYaw = LookDir.Rotation().Yaw;

    StartTime = FPlatformTime::Seconds();

    // Duration이 0이면 즉시 회전 적용
    if (Duration <= 0.f)
    {
        SubjectActor->SetActorRotation(FRotator(0.f, TargetYaw, 0.f));
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        RotationTimerHandle,
        this,
        &UPBDNodeFeature_FaceTarget::TickRotation,
        0.016f,
        true
    );
}

void UPBDNodeFeature_FaceTarget::OnEndDialogueNode_Implementation(const UDialogueNode* InDialogueNode, const FDialogueSystemContext& InDialogueContext)
{
    Super::OnEndDialogueNode_Implementation(InDialogueNode, InDialogueContext);
    
    if (RotationTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
    }
    
    AActor* SubjectActor = FindParticipantActor(InDialogueContext, SubjectParticipantTag);
    AActor* TargetActor = FindParticipantActor(InDialogueContext, TargetParticipantTag);

    if (!IsValid(SubjectActor) || !IsValid(TargetActor) || SubjectActor == TargetActor)
    {
        return;
    }

    SubjectActorRef = SubjectActor;
    StartYaw = SubjectActor->GetActorRotation().Yaw;

    // 수평면 기준 바라볼 방향의 Yaw 계산
    const FVector LookDir = (TargetActor->GetActorLocation() - SubjectActor->GetActorLocation()).GetSafeNormal2D();
    TargetYaw = LookDir.Rotation().Yaw;

    StartTime = FPlatformTime::Seconds();

    // 즉시 회전 적용
    SubjectActor->SetActorRotation(FRotator(0.f, TargetYaw, 0.f));
}

void UPBDNodeFeature_FaceTarget::TickRotation()
{
    if (!SubjectActorRef.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
        return;
    }

    const float Elapsed = static_cast<float>(FPlatformTime::Seconds() - StartTime);
    const float Alpha = FMath::Clamp(Elapsed / Duration, 0.f, 1.f);

    // 최단 경로 Yaw 보간 (±180도 경계 처리)
    const float DeltaYaw = FRotator::NormalizeAxis(TargetYaw - StartYaw);
    const float CurrentYaw = FRotator::NormalizeAxis(StartYaw + DeltaYaw * Alpha);
    SubjectActorRef->SetActorRotation(FRotator(0.f, CurrentYaw, 0.f));

    if (Alpha >= 1.f)
    {
        GetWorld()->GetTimerManager().ClearTimer(RotationTimerHandle);
    }
}
