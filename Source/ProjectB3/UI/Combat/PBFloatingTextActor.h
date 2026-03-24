// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBFloatingTextActor.generated.h"

class UWidgetComponent;
class UPBFloatingTextPayload;
class UPBFloatingTextWidget;

/**
 * 월드에 스폰되어 플로팅 텍스트 위젯을 표시하고 자동 소멸하는 경량 액터.
 * ShowFloatingText 어빌리티로부터 스폰되며, 위젯 애니메이션 종료 후 Destroy된다.
 */
UCLASS()
class PROJECTB3_API APBFloatingTextActor : public AActor
{
	GENERATED_BODY()

public:
	APBFloatingTextActor();

	// 페이로드와 위젯 클래스로 초기화 (SpawnActorDeferred 후 FinishSpawning 전 호출)
	void InitWithPayload(UPBFloatingTextPayload* Payload, TSubclassOf<UPBFloatingTextWidget> WidgetClass);

	// 카메라 기준 오프셋 추적에 필요한 소유자 및 오프셋 설정
	void SetFollowParams(AActor* InFollowTarget, float InOffset);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	// 위젯 애니메이션 종료 콜백
	UFUNCTION()
	void OnWidgetAnimationFinished();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> WidgetComp;

private:
	// 초기화 완료 여부
	bool bInitialized = false;

	// 위치 추적 대상 (AvatarActor)
	TWeakObjectPtr<AActor> FollowTarget;

	// 카메라 Up 방향 기준 오프셋 크기
	float FollowOffset = 0.f;

	// 초기화 시 캐시된 데이터
	UPROPERTY()
	TObjectPtr<UPBFloatingTextPayload> CachedPayload;

	UPROPERTY()
	TSubclassOf<UPBFloatingTextWidget> CachedWidgetClass;
};
