// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBProjectile.h"
#include "GameplayEffectTypes.h"
#include "PBArrowProjectile.generated.h"

// 화살 명중 또는 소멸(사거리 초과·장애물 충돌) 시 브로드캐스트
DECLARE_DELEGATE_OneParam(FOnArrowResolved, AActor*);

// 화살 투사체. 지정된 대상에 명중 시 데미지 이펙트를 적용하고 어빌리티에 알린다.
// 대상 외 충돌 또는 사거리 초과 시 데미지 미적용으로 어빌리티에 알린다.
UCLASS()
class PROJECTB3_API APBArrowProjectile : public APBProjectile
{
	GENERATED_BODY()

public:
	// 발사 전 데미지 스펙·발사자 ASC·대상 액터를 설정
	void SetupArrow(
		const FGameplayEffectSpecHandle& InDamageSpec,
		UAbilitySystemComponent* InSourceASC,
		AActor* InTargetActor,
		float InMaxRange);

protected:
	/*~ APBProjectile Interface ~*/
	// 충돌 시 대상 여부 확인 후 데미지 적용, OnArrowResolved 브로드캐스트
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	// 사거리 초과 시 데미지 미적용으로 OnArrowResolved 브로드캐스트 후 Destroy
	virtual void OnRangeExceeded() override;

public:
	// 명중 또는 소멸 시 브로드캐스트 (어빌리티 종료 트리거)
	FOnArrowResolved OnArrowResolved;
	
private:
	// 적용할 데미지 GE 스펙
	FGameplayEffectSpecHandle DamageSpecHandle;

	// 발사자 AbilitySystemComponent
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	// 타겟 액터 (이 액터에만 데미지 적용)
	TWeakObjectPtr<AActor> TargetActor;
};
