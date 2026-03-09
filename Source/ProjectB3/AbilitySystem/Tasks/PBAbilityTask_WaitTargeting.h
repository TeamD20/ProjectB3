// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBAbilityTask_WaitTargeting.generated.h"

class UPBTargetingComponent;
class APBGameplayPlayerController;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnWaitTargetingConfirmed, const FPBAbilityTargetData&);
DECLARE_MULTICAST_DELEGATE(FOnWaitTargetingCancelled);

/**
 * 플레이어 타겟팅 세션을 UPBGameplayAbility_Targeted와 UPBTargetingComponent 사이에서 중개하는 AbilityTask.
 */
UCLASS()
class PROJECTB3_API UPBAbilityTask_WaitTargeting : public UAbilityTask
{
	GENERATED_BODY()

public:
	// Task 생성
	static UPBAbilityTask_WaitTargeting* CreateTask(UGameplayAbility* OwningAbility);

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	// TargetingComponent 콜백 핸들러
	void HandleTargetConfirmed(const FPBAbilityTargetData& TargetData);
	void HandleTargetCancelled();

public:
	// 타겟 확정 델리게이트
	FOnWaitTargetingConfirmed OnTargetConfirmed;

	// 타겟팅 취소 델리게이트
	FOnWaitTargetingCancelled OnTargetCancelled;

private:
	// 바인딩된 PC (약참조)
	TWeakObjectPtr<APBGameplayPlayerController> WeakPC;

	// 바인딩된 TargetingComponent (약참조)
	TWeakObjectPtr<UPBTargetingComponent> WeakTargetingComp;
};
