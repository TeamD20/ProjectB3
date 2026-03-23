// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBGameplayAbility_Targeted.h"
#include "ProjectB3/Combat/Projectiles/PBArrowProjectile.h"
#include "PBGameplayAbility_RangedAttack.generated.h"

class APBEquipmentActor;

// 투사체 발사 어빌리티 기반 클래스.
// BP에서 K2_ExecuteTargetLogic을 override하고, FireProjectile()를 호출하여 투사체 발사.
// 투사체 도착·소멸 시 EndAbility 내부 처리 (EndMode = Manual).
UCLASS()
class PROJECTB3_API UPBGameplayAbility_RangedAttack : public UPBGameplayAbility_Targeted
{
	GENERATED_BODY()

public:
	// 전달받은 데미지 스펙으로 지정 위치를 향해 투사체 발사. 어빌리티 종료는 내부에서 처리.
	// DamageSpecHandle: 외부(BP)에서 사전 계산된 데미지 GE 스펙. TargetLocation: 투사체 조준 위치.
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ability|RangedAttack")
	void FireProjectile(const FGameplayEffectSpecHandle& DamageSpecHandle, const FVector& TargetLocation, AActor* TargetActor);

	/*~ UPBGameplayAbility_Targeted Interface ~*/
	// 타겟팅 요청에 투사체 경로 프리뷰 컨텍스트 추가
	virtual FPBTargetingRequest MakeTargetingRequest() const override;
	// 무기 소켓 기반 발사 지점 반환 (매 프레임 갱신)
	virtual FVector GetProjectileLaunchLocation() const override;

private:
	// DynamicTags의 장비 슬롯 태그 기반으로 현재 장착 무기 액터 반환. 없으면 nullptr.
	APBEquipmentActor* GetEquippedWeaponActor() const;

	// 투사체 발사 기점 Transform 결정 (무기 소켓 → 폴백: 캐릭터 위치)
	FTransform GetProjectileLaunchTransform() const;

	// 투사체 도착 시 호출되는 콜백 — EndAbility 트리거
	void OnProjectileResolved(AActor* HitActor);

private:
	// 발사할 투사체 클래스 (블루프린트에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "Ability|Projectile")
	TSubclassOf<APBProjectile> ProjectileClass;

	// 투사체 콜백에서 EndAbility 호출 시 사용하는 캐시
	FGameplayAbilitySpecHandle CachedHandle;
	FGameplayAbilityActorInfo CachedActorInfo;
	FGameplayAbilityActivationInfo CachedActivationInfo;
};
