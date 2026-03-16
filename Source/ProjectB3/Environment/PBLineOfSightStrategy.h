// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBEnvironmentTypes.h"
#include "PBLineOfSightStrategy.generated.h"

/**
 * LoS 판정 전략 베이스 클래스.
 * 순수 판정만 수행하며 캐싱은 EnvironmentSubsystem이 전담한다.
 * 구현체는 필요한 내부 데이터(장애물 맵 등)를 자유롭게 관리할 수 있다.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTB3_API UPBLineOfSightStrategy : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 두 위치 간 시야 판정 수행.
	 * @param World     월드 컨텍스트
	 * @param Source    시전자 위치 (월드 좌표)
	 * @param Target    대상 액터
	 * @return          시야 판정 결과
	 */
	virtual FPBLoSResult Execute(
		const UWorld* World,
		const FVector& Source,
		const AActor* Target
	) const PURE_VIRTUAL(UPBLineOfSightStrategy::Execute, return FPBLoSResult(););
};
