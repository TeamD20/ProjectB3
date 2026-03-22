// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBGameplayAbility.h"
#include "PBGameplayAbility_UseConsumable.generated.h"

class UPBInventoryComponent;

/**
 * 소비 아이템 사용을 전담하는 영구 패시브 어빌리티.
 * 캐릭터에 한 번만 부여되며, Event.Item.UseConsumable GameplayEvent로 트리거된다.
 *
 * 동작 흐름:
 *   1. FGameplayEventData.OptionalObject에서 UPBConsumableUsePayload 추출
 *   2. ASC.OnPBAbilityEnded 구독
 *   3. HandleGameplayEvent의 AbilityScopeLock 해제 이후(NextTick) GiveAbilityAndActivateOnce 호출
 *   4. 서브 어빌리티 종료 시 스택 차감 (취소 아닌 경우에만)
 *      — BonusAction 차감은 서브 어빌리티 자체의 Cost 태그가 처리
 */
UCLASS()
class PROJECTB3_API UPBGameplayAbility_UseConsumable : public UPBGameplayAbility
{
	GENERATED_BODY()

public:
	UPBGameplayAbility_UseConsumable();

protected:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	// 서브 어빌리티 종료 콜백 — OnPBAbilityEnded에 바인딩
	void OnSubAbilityEnded(FGameplayAbilitySpecHandle EndedHandle, bool bWasCancelled);

	// 다음 틱에 서브 어빌리티 부여 + 발동 — HandleGameplayEvent의 AbilityScopeLock 해제 이후 실행
	void ActivateSubAbilityNextTick();

private:
	// 현재 감시 중인 서브 어빌리티 핸들
	FGameplayAbilitySpecHandle PendingSubAbilityHandle;

	// 사용 중인 아이템 인스턴스 ID
	FGuid PendingInstanceID;

	// 다음 틱에 부여할 서브 어빌리티 클래스
	UPROPERTY()
	TSubclassOf<UPBGameplayAbility> PendingSubAbilityClass;
};
