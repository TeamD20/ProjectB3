// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBCombatStateTextActor.generated.h"

class UWidgetComponent;
class UPBCombatStateTextWidget;

/**
 * 월드에 스폰되어 전투 진입/종료 텍스트 위젯을 표시하고 위젯의 애니메이션이 종료되면 자동 소멸하는 경량 액터.
 */
UCLASS()
class PROJECTB3_API APBCombatStateTextActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APBCombatStateTextActor();

	// 스폰 직후 초기화 호출 
	void InitCombatState(bool bIsStarting, TSubclassOf<UPBCombatStateTextWidget> WidgetClass);

protected:
	virtual void BeginPlay() override;

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
	bool bInitialized = false;
	bool bCachedIsStarting = false;

	UPROPERTY()
	TSubclassOf<UPBCombatStateTextWidget> CachedWidgetClass;
};
