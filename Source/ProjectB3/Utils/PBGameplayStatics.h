// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "PBGameplayStatics.generated.h"

class UMeshComponent;
class AController;

// 프로젝트 전역 유틸리티 함수 모음. C++ 및 Blueprint 양쪽에서 호출 가능한 정적 헬퍼 함수 제공.
UCLASS()
class PROJECTB3_API UPBGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 액터 및 모든 ChildActor를 재귀 탐색하여 UMeshComponent를 OutMeshes에 수집 */
	UFUNCTION(BlueprintCallable, Category = "PB|Utils")
	static void GetAllMeshComponents(AActor* Actor, TArray<UMeshComponent*>& OutMeshes);

	/** 부분 경로를 허용하지 않는 이동 요청 */
	UFUNCTION(BlueprintCallable, Category = "PB|Navigation")
	static bool SimpleMoveToLocation(AController* Controller, const FVector& GoalLocation, float AcceptanceRadius = 50.f);
};