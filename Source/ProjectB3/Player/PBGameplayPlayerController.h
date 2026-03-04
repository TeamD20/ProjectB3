// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "PBGameplayPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnViewDataPropertyChangedSiganture);

class UPBPathDisplayComponent;
class UInputMappingContext;
class UInputAction;

// 플레이어 컨트롤러 기반 클래스. 입력 처리, 플레이어 폰 제어 로직 담당.
UCLASS()
class PROJECTB3_API APBGameplayPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APBGameplayPlayerController();

	// 목표 위치까지의 NavPath를 쿼리하고 경로 시각화를 갱신
	UFUNCTION(BlueprintCallable, Category = "PathDisplay")
	void RequestNavPathDisplay(const FVector& TargetLocation);

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/*~ APlayerController Interface ~*/
	virtual void SetupInputComponent() override;

private:
	// 마우스 클릭 시 폰을 해당 위치로 이동 명령
	void OnMoveCommandTriggered(const FInputActionValue& Value);

	// 매 틱마다 커서 아래 위치로 경로 미리보기를 갱신
	void UpdateHoverPathDisplay();

	// 경로 포인트 배열에서 MaxMoveDistance 기준으로 실제 이동 목적지를 계산
	// 전체 경로가 MaxMoveDistance보다 짧으면 경로 끝점을 반환
	FVector CalculateClampedDestination(const TArray<FVector>& PathPoints) const;

public:
	/*~ Input Settings ~*/

	// 기본 입력 매핑 컨텍스트
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// 이동 명령 입력 액션 (좌클릭)
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveCommandAction;

	/*~ Movement Settings ~*/

	// 캐릭터의 최대 이동 가능 거리
	UPROPERTY(EditAnywhere, Category = "Movement")
	float MaxMoveDistance = 600.0f;

	/*~ PathDisplay Settings ~*/

	// 경로 갱신을 트리거하는 최소 커서 이동 거리
	UPROPERTY(EditAnywhere, Category = "PathDisplay")
	float PathUpdateMinDistance = 20.0f;

private:
	// 이동 경로 시각화 컴포넌트
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPBPathDisplayComponent> PathDisplayComponent;

	// 마지막으로 경로를 갱신한 커서 위치
	FVector LastHoverLocation = FVector::ZeroVector;
};
