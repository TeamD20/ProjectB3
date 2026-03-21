// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDialogueCameraActor.h"

APBDialogueCameraActor::APBDialogueCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

void APBDialogueCameraActor::UpdateTracking(AActor* InSpeaker, AActor* InListener, const FPBDialogueCameraParams& InParams)
{
    TrackedSpeaker = InSpeaker;
    TrackedListener = InListener;
    CameraParams = InParams;

    // 틱 활성화 및 즉시 위치 갱신
    SetActorTickEnabled(true);
    Tick(0.f);
}

void APBDialogueCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!TrackedSpeaker.IsValid())
    {
        return;
    }

    const FVector SpeakerLoc = TrackedSpeaker->GetActorLocation();

    // 청자 위치: 청자 Actor가 없으면 화자 전방 역방향을 Fallback으로 사용
    FVector ListenerLoc;
    if (TrackedListener.IsValid())
    {
        ListenerLoc = TrackedListener->GetActorLocation();
    }
    else
    {
        ListenerLoc = SpeakerLoc + TrackedSpeaker->GetActorForwardVector() * CameraParams.ArmLength;
    }

    const FVector FinalLoc = CalcDesiredLocation(SpeakerLoc, ListenerLoc);
    const FVector SpeakerHeadLoc = GetSpeakerHeadLocation(SpeakerLoc);

    // 화자 머리를 향하는 LookAt 회전
    const FRotator LookAt = (SpeakerHeadLoc - FinalLoc).GetSafeNormal().Rotation();

    SetActorLocationAndRotation(FinalLoc, LookAt);
}

FVector APBDialogueCameraActor::CalcDesiredLocation(const FVector& SpeakerLoc, const FVector& ListenerLoc) const
{
    // 청자의 실제 Forward/Right 벡터를 사용해 Over-the-Shoulder 구도 결정.
    // Fallback: 청자가 없을 경우 화자-청자 위치 벡터 기반으로 계산
    FVector BackDir;
    FVector RightDir;

    if (TrackedListener.IsValid())
    {
        // 청자 등 뒤 방향 (Forward 역방향)
        BackDir  = -TrackedListener->GetActorForwardVector();
        RightDir =  TrackedListener->GetActorRightVector();
    }
    else
    {
        // 화자 -> 청자 방향의 역방향
        BackDir  = -(SpeakerLoc - ListenerLoc).GetSafeNormal2D();
        RightDir =  FVector::CrossProduct(FVector::UpVector, BackDir).GetSafeNormal();
    }

    // Over-the-Shoulder 희망 위치
    const FVector DesiredLoc = ListenerLoc
        + BackDir  * CameraParams.ArmLength
        + RightDir * CameraParams.SideOffset
        + FVector::UpVector * CameraParams.HeightOffset;

    // SphereTrace: 화자 머리 -> 희망 위치. 충돌 시 카메라를 화자 방향으로 당김
    const FVector SpeakerHeadLoc = GetSpeakerHeadLocation(SpeakerLoc);

    FHitResult Hit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (TrackedSpeaker.IsValid())
    {
        QueryParams.AddIgnoredActor(TrackedSpeaker.Get());
    }
    if (TrackedListener.IsValid())
    {
        QueryParams.AddIgnoredActor(TrackedListener.Get());
    }

    const bool bHit = GetWorld()->SweepSingleByChannel(
        Hit,
        SpeakerHeadLoc,
        DesiredLoc,
        FQuat::Identity,
        ECC_Camera,
        FCollisionShape::MakeSphere(CameraParams.ProbeRadius),
        QueryParams
    );

    if (bHit)
    {
        // 충돌 지점에서 법선 방향으로 ProbeRadius만큼 띄워 벽 관통 방지
        return Hit.Location + Hit.Normal * CameraParams.ProbeRadius;
    }

    return DesiredLoc;
}

FVector APBDialogueCameraActor::GetSpeakerHeadLocation(const FVector& SpeakerLoc) const
{
    return SpeakerLoc + FVector::UpVector * HeadHeightOffset;
}
