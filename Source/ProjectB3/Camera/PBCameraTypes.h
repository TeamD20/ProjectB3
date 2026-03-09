// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "PBCameraTypes.generated.h"

// 게임 모드별 카메라 조작 값 제한을 선언하는 구조체.
// 입력 허용/차단은 IMC가 담당하고, 이 구조체는 수치적 제약만 정의한다.
USTRUCT(BlueprintType)
struct FPBCameraModeParams
{
	GENERATED_BODY()

	// 허용 최소 Yaw (-1 = 무제한)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation")
	float RotateRangeMin = -1.0f;

	// 허용 최대 Yaw (-1 = 무제한)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rotation")
	float RotateRangeMax = -1.0f;

	// 허용 최소 줌 레벨 (0.0 = 최대 줌아웃)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ZoomMin = 0.0f;

	// 허용 최대 줌 레벨 (1.0 = 최대 줌인)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ZoomMax = 1.0f;

	// 이동 속도 배율 (1.0 = 기본)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Move", meta = (ClampMin = "0.0"))
	float MoveSpeedScale = 1.0f;

	// 구면 줌 활성화 (true: Pitch 연동, false: 거리만)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom")
	bool bUseSphericalZoom = true;
};
