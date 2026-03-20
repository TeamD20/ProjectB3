// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBTacticalCameraTypes.generated.h"

/** 전술 카메라 동작 모드 */
UENUM(BlueprintType)
enum class EPBTacticalCameraMode : uint8
{
	// 전술 카메라 비활성
	Inactive,

	// 적 턴 추적
	EnemyTurn,

	// 스킬 시전 프레이밍
	SkillFraming,
};

/** HUD 패딩 비율 — 방향별 독립 적용 */
USTRUCT(BlueprintType)
struct FPBScreenPadding
{
	GENERATED_BODY()

	// 상단 패딩 비율 (턴 오더 UI)
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float Top = 0.1f;

	// 하단 패딩 비율 (액션바 / 스킬바)
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float Bottom = 0.25f;

	// 좌측 패딩 비율 (파티 프레임)
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float Left = 0.1f;

	// 우측 패딩 비율
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float Right = 0.05f;
};

/** 전술 카메라 추적 대상 정보 */
USTRUCT()
struct FPBTacticalCameraTarget
{
	GENERATED_BODY()

	// 주 피사체 (적 캐릭터 / 시전자)
	TWeakObjectPtr<AActor> PrimarySubject;

	// 보조 피사체 (스킬 대상, 없으면 nullptr)
	TWeakObjectPtr<AActor> SecondarySubject;

	// 카메라가 바라볼 초점
	FVector FocusPoint = FVector::ZeroVector;

	// 초점으로부터 카메라 거리
	float DesiredDistance = 1500.0f;

	// 카메라 Pitch각
	float DesiredPitch = -50.0f;

	// 매 틱 PrimarySubject 위치로 FocusPoint 갱신 여부
	bool bTrackPrimarySubject = false;
};
