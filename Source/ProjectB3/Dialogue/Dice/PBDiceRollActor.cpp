// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBDiceRollActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

// 수학적으로 계산된 정이십면체 20개 면의 단위 법선 벡터 (표준 배치 기준)
// 반대 방향 쌍: (0↔17), (1↔16), (2↔15), (3↔19), (4↔18), (5↔10), (6↔11), (7↔12), (8↔13), (9↔14)
const TArray<FVector> APBDiceRollActor::D20FaceNormals =
{
    FVector(+0.356822f, +0.000000f, +0.934172f),  // Face[0]
    FVector(+0.577350f, +0.577350f, +0.577350f),  // Face[1]
    FVector(+0.000000f, +0.934172f, +0.356822f),  // Face[2]
    FVector(-0.577350f, +0.577350f, +0.577350f),  // Face[3]
    FVector(-0.356822f, +0.000000f, +0.934172f),  // Face[4]
    FVector(+0.577350f, -0.577350f, +0.577350f),  // Face[5]
    FVector(+0.934172f, -0.356822f, +0.000000f),  // Face[6]
    FVector(+0.934172f, +0.356822f, +0.000000f),  // Face[7]
    FVector(+0.577350f, +0.577350f, -0.577350f),  // Face[8]
    FVector(+0.000000f, +0.934172f, -0.356822f),  // Face[9]
    FVector(-0.577350f, +0.577350f, -0.577350f),  // Face[10]
    FVector(-0.934172f, +0.356822f, +0.000000f),  // Face[11]
    FVector(-0.934172f, -0.356822f, +0.000000f),  // Face[12]
    FVector(-0.577350f, -0.577350f, +0.577350f),  // Face[13]
    FVector(+0.000000f, -0.934172f, +0.356822f),  // Face[14]
    FVector(+0.000000f, -0.934172f, -0.356822f),  // Face[15]
    FVector(-0.577350f, -0.577350f, -0.577350f),  // Face[16]
    FVector(-0.356822f, +0.000000f, -0.934172f),  // Face[17]
    FVector(+0.356822f, +0.000000f, -0.934172f),  // Face[18]
    FVector(+0.577350f, -0.577350f, -0.577350f),  // Face[19]
};

APBDiceRollActor::APBDiceRollActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    DiceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DiceMesh"));
    DiceMesh->SetupAttachment(SceneRoot);

    for (int32 Index = 0; Index < 20; ++Index)
    {
        FaceIndexToNumber.Add(Index, Index + 1);
    }
}

void APBDiceRollActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    switch (RollPhase)
    {
    case EPBDiceRollPhase::Rolling:
        TickRolling(DeltaTime);
        break;
    case EPBDiceRollPhase::Settling:
        TickSettling(DeltaTime);
        break;
    default:
        break;
    }
}

void APBDiceRollActor::RollToNumber(int32 TargetNumber, float RollDuration, float SettleDuration)
{
    if (!IsValid(DiceMesh))
    {
        return;
    }

    PendingTargetNumber = TargetNumber;
    RollingDuration     = FMath::Max(RollDuration, 0.f);
    SettlingDuration    = FMath::Max(SettleDuration, 0.01f);
    RollingElapsed      = 0.f;
    SettlingElapsed     = 0.f;

    // 초기 각속도 랜덤 결정
    auto GetRandomSpeed = [this]()
    {
        const float Speed = FMath::RandRange(MinAngularSpeed, MaxAngularSpeed);
        return FMath::RandBool() ? Speed : -Speed;
    };
    RollAngularVelocity   = FRotator(GetRandomSpeed(), GetRandomSpeed(), GetRandomSpeed());
    TargetAngularVelocity = RollAngularVelocity;
    VelocityChangeTimer   = VelocityChangeInterval;

    RollPhase = EPBDiceRollPhase::Rolling;
    SetActorTickEnabled(true);
}

void APBDiceRollActor::TickRolling(float DeltaTime)
{
    RollingElapsed += DeltaTime;

    // 일정 주기마다 목표 각속도를 새로 뽑아 부드럽게 보간 → 방향이 자연스럽게 변화
    VelocityChangeTimer -= DeltaTime;
    if (VelocityChangeTimer <= 0.f)
    {
        auto GetRandomSpeed = [this]()
        {
            const float Speed = FMath::RandRange(MinAngularSpeed, MaxAngularSpeed);
            return FMath::RandBool() ? Speed : -Speed;
        };
        TargetAngularVelocity = FRotator(GetRandomSpeed(), GetRandomSpeed(), GetRandomSpeed());
        VelocityChangeTimer   = VelocityChangeInterval;
    }

    // 현재 각속도를 목표로 부드럽게 보간 (급격한 방향 전환 없이 연속적으로 구름)
    RollAngularVelocity = FMath::RInterpTo(RollAngularVelocity, TargetAngularVelocity, DeltaTime, VelocityInterpSpeed);

    const FQuat DeltaQuat = (RollAngularVelocity * DeltaTime).Quaternion();
    DiceMesh->SetRelativeRotation((DeltaQuat * DiceMesh->GetRelativeRotation().Quaternion()).Rotator());

    if (RollingElapsed >= RollingDuration)
    {
        SettlingStartQuat  = DiceMesh->GetRelativeRotation().Quaternion();
        SettlingTargetQuat = CalcTargetQuat(PendingTargetNumber);

        // Slerp 최단경로 스냅 방지: 같은 반구이면 목표를 반전하여 긴 경로(한 바퀴 더)로 수렴
        if ((SettlingStartQuat | SettlingTargetQuat) > 0.f)
        {
            SettlingTargetQuat = -SettlingTargetQuat;
        }

        SettlingElapsed = 0.f;
        RollPhase       = EPBDiceRollPhase::Settling;
    }
}

void APBDiceRollActor::TickSettling(float DeltaTime)
{
    SettlingElapsed += DeltaTime;

    // EaseOut 커브로 자연스럽게 감속 수렴
    const float Alpha      = FMath::Clamp(SettlingElapsed / SettlingDuration, 0.f, 1.f);
    const float EasedAlpha = 1.f - FMath::Square(1.f - Alpha);

    DiceMesh->SetRelativeRotation(FQuat::Slerp(SettlingStartQuat, SettlingTargetQuat, EasedAlpha).Rotator());

    if (Alpha >= 1.f)
    {
        // 오차 없이 정확한 목표 회전으로 고정
        DiceMesh->SetRelativeRotation(SettlingTargetQuat.Rotator());

        RollPhase = EPBDiceRollPhase::Idle;
        SetActorTickEnabled(false);

        OnDiceRollFinished.Broadcast(PendingTargetNumber);
    }
}

FQuat APBDiceRollActor::CalcTargetQuat(int32 DiceNumber) const
{
    for (const TPair<int32, int32>& Pair : FaceIndexToNumber)
    {
        if (Pair.Value == DiceNumber)
        {
            const int32 FaceIndex = Pair.Key;
            if (!D20FaceNormals.IsValidIndex(FaceIndex))
            {
                break;
            }
            const FVector CorrectedNormal = MeshRotationOffset.RotateVector(D20FaceNormals[FaceIndex]);
            return FQuat::FindBetweenNormals(CorrectedNormal, FVector::ForwardVector);
        }
    }

    return FQuat::Identity;
}

void APBDiceRollActor::AlignFaceToForward(int32 FaceIndex)
{
    if (!D20FaceNormals.IsValidIndex(FaceIndex))
    {
        return;
    }

    const FVector CorrectedNormal = MeshRotationOffset.RotateVector(D20FaceNormals[FaceIndex]);
    const FQuat AlignQuat = FQuat::FindBetweenNormals(CorrectedNormal, FVector::ForwardVector);

    if (IsValid(DiceMesh))
    {
        DiceMesh->SetRelativeRotation(AlignQuat.Rotator());
    }
}

void APBDiceRollActor::AlignDiceNumberToForward(int32 DiceNumber)
{
    for (const TPair<int32, int32>& Pair : FaceIndexToNumber)
    {
        if (Pair.Value == DiceNumber)
        {
            AlignFaceToForward(Pair.Key);
            return;
        }
    }
}

int32 APBDiceRollActor::GetFaceIndexFacingForward() const
{
    if (!IsValid(DiceMesh))
    {
        return INDEX_NONE;
    }

    const FRotator MeshWorldRot = DiceMesh->GetComponentRotation();

    int32 BestFace = 0;
    float BestDot  = -2.f;

    for (int32 i = 0; i < D20FaceNormals.Num(); i++)
    {
        const FVector CorrectedNormal = MeshRotationOffset.RotateVector(D20FaceNormals[i]);
        const FVector WorldNormal     = MeshWorldRot.RotateVector(CorrectedNormal);
        const float Dot               = FVector::DotProduct(WorldNormal, GetActorForwardVector());
        if (Dot > BestDot)
        {
            BestDot  = Dot;
            BestFace = i;
        }
    }

    return BestFace;
}

int32 APBDiceRollActor::GetDiceNumberFacingForward() const
{
    const int32 FaceIndex = GetFaceIndexFacingForward();
    if (FaceIndex == INDEX_NONE)
    {
        return -1;
    }

    const int32* Found = FaceIndexToNumber.Find(FaceIndex);
    return Found ? *Found : -1;
}

FVector APBDiceRollActor::GetFaceNormalInWorldSpace(int32 FaceIndex) const
{
    if (!D20FaceNormals.IsValidIndex(FaceIndex) || !IsValid(DiceMesh))
    {
        return FVector::ZeroVector;
    }

    const FVector CorrectedNormal = MeshRotationOffset.RotateVector(D20FaceNormals[FaceIndex]);
    return DiceMesh->GetComponentRotation().RotateVector(CorrectedNormal);
}
