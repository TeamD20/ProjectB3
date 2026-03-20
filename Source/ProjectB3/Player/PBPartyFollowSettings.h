// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPath.h"
#include "PBPartyFollowSettings.generated.h"

/**
 * 파티 추적 기능 설정을 에디터(Project Settings)에 노출하기 위한 클래스입니다.
 */
UCLASS(Config=Game, defaultconfig, meta=(DisplayName="Party Follow Settings"))
class PROJECTB3_API UPBPartyFollowSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPBPartyFollowSettings();

	// Trail Point 기록 및 팔로워 디스패치 주기 (초)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Trail")
	float TrailRecordInterval = 0.2f;

	// Trail Point 기록 최소 이동 거리 (cm)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Trail")
	float MinTrailDistance = 100.f;

	// 팔로워 간 Trail Point 간격 (Ring Buffer 인덱스 수)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Trail")
	int32 TrailSpacing = 4;

	// 추적 종료 (히스테리시스 하한) 거리 (cm)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Trail")
	float StopDistance = 200.f;

	// Scatter 배치 최소(내부) 반경 (cm)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Scatter")
	float ScatterInnerRadius = 100.f;

	// Scatter 배치 최대(외부) 반경 (cm)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Scatter")
	float ScatterRadius = 300.f;

	// Scatter 위치 간 최소 간격 (cm)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Scatter")
	float ScatterMinSpacing = 120.f;

	// Scatter 랜덤 오프셋 최대값 (cm)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Scatter")
	float ScatterRandomOffset = 30.f;

	// 연속 실패 후 리더 직행 Fallback 임계 횟수
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Fallback")
	int32 FallbackFailThreshold = 3;

	// Scatter 위치 계산에 사용할 EQS 쿼리 에셋 (없으면 C++ NavMesh 원형 후보 사용)
	UPROPERTY(config, EditAnywhere, Category = "PartyFollow|Scatter", meta=(AllowedClasses="/Script/AIModule.EnvQuery"))
	FSoftObjectPath ScatterEQSQueryPath;
};
