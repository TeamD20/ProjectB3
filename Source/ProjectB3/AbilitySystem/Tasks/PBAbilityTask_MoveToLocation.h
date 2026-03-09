// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "Navigation/PathFollowingComponent.h"
#include "PBAbilityTask_MoveToLocation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPBMoveCompleted, TEnumAsByte<EPathFollowingResult::Type>, Result);

/**
 * PathFollowingComponent를 활용하여 NavMesh 경로를 따라 이동하는 AbilityTask.
 * AIController·PlayerController 모두 지원한다.
 * 이동 완료·실패·중단 시 OnMoveCompleted를 브로드캐스트한다.
 */
UCLASS()
class PROJECTB3_API UPBAbilityTask_MoveToLocation : public UAbilityTask
{
	GENERATED_BODY()

public:
	// Task 생성
	static UPBAbilityTask_MoveToLocation* CreateTask(UGameplayAbility* OwningAbility, FVector InDestination);

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

public:
	// 이동 완료/실패 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnPBMoveCompleted OnMoveCompleted;

private:
	// PathFollowingComponent의 이동 완료 핸들러
	void HandleRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	// 목표 위치
	FVector Destination;

	// 이동 요청 ID
	FAIRequestID MoveRequestID;

	// 바인딩된 PathFollowingComponent (약참조)
	TWeakObjectPtr<UPathFollowingComponent> WeakPathFollowingComp;
};
