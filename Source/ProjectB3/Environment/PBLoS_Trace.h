// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBLineOfSightStrategy.h"
#include "PBLoS_Trace.generated.h"

/**
 * UE5 물리 LineTrace 기반 LoS 전략.
 * 가장 정밀하며, 비정형 3D 지형에 강점.
 * 내부 데이터 불필요 — World의 콜리전 데이터를 직접 참조한다.
 */
UCLASS()
class PROJECTB3_API UPBLoS_Trace : public UPBLineOfSightStrategy
{
	GENERATED_BODY()

public:
	/*~ UPBLineOfSightStrategy Interface ~*/
	virtual FPBLoSResult Execute(
		const UWorld* World,
		const FVector& Source,
		const AActor* Target
	) const override;

private:
	// Trace 시작/끝 높이 오프셋 (캐릭터 중심 높이, cm)
	static constexpr float TraceHeightOffset = 90.0f;

	// 고지대 판정 기준 고도 차 (cm, BG3 기준 2.5m = 250cm)
	static constexpr float HighGroundThreshold = 250.0f;
};
