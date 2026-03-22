// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "PBDialogueCameraActor.generated.h"

/**
 * 대화 연출 전용 카메라 액터.
 * 화자(Speaker)를 매 틱 추적하며 Over-the-Shoulder 구도를 유지한다.
 * 벽 가림 발생 시 SphereTrace로 카메라를 화자 방향으로 당긴다.
 */
UCLASS()
class PROJECTB3_API APBDialogueCameraActor : public ACameraActor
{
    GENERATED_BODY()

public:
    APBDialogueCameraActor();

    /*~ APBDialogueCameraActor Interface ~*/
    /** 추적 대상 및 카메라 파라미터 갱신 후 즉시 위치 재계산 */
    void UpdateTracking(AActor* InSpeaker, AActor* InListener, const FPBDialogueCameraParams& InParams);

    /*~ AActor Interface ~*/
    virtual void Tick(float DeltaTime) override;

private:
    /** Over-the-Shoulder 기반 희망 카메라 위치 계산 (SphereTrace 보정 포함) */
    FVector CalcDesiredLocation(const FVector& SpeakerLoc, const FVector& ListenerLoc) const;

    /** Speaker 머리 위치 반환 (Actor 위치 + 고정 높이 오프셋) */
    FVector GetSpeakerHeadLocation(const FVector& SpeakerLoc) const;

private:
    // 추적 중인 화자
    TWeakObjectPtr<AActor> TrackedSpeaker;

    // 추적 중인 청자 (없으면 화자 ForwardVector 역방향 사용)
    TWeakObjectPtr<AActor> TrackedListener;

    // 카메라 배치 파라미터
    FPBDialogueCameraParams CameraParams;

    // 화자 머리 기준 고정 높이 오프셋 (cm)
    static constexpr float HeadHeightOffset = 50.f;
};
