// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBSkillNameFloatingActor.generated.h"

class UWidgetComponent;
class UPBSkillNameFloatingWidget;

/**
 * 스킬 이름 위젯을 월드에 표시하고 애니메이션 종료 시 자동 소멸하는 경량 액터
 */
UCLASS()
class PROJECTB3_API APBSkillNameFloatingActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APBSkillNameFloatingActor();

	// 초기화
	void InitSkillName(const FText& SkillName, EPBAbilityType AbilityType, TSubclassOf<UPBSkillNameFloatingWidget> WidgetClass);

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

	FText CachedSkillName;
	EPBAbilityType CachedAbilityType = EPBAbilityType::None;

	UPROPERTY()
	TSubclassOf<UPBSkillNameFloatingWidget> CachedWidgetClass;
};
