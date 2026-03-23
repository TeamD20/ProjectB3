// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBProjectile.h"
#include "GameplayEffectTypes.h"
#include "PBArrowProjectile.generated.h"

// 화살 도착 또는 소멸 시 브로드캐스트
DECLARE_DELEGATE_OneParam(FOnArrowResolved, AActor*);

// 화살 투사체. Bezier 곡선 비행 후 타겟 도착 시 데미지 이펙트를 적용하고 어빌리티에 알린다.
UCLASS()
class PROJECTB3_API APBArrowProjectile : public APBProjectile
{
	GENERATED_BODY()

public:
	// 발사 전 데미지 스펙·발사자 ASC·대상 액터를 설정
	void SetupArrow(
		const FGameplayEffectSpecHandle& InDamageSpec,
		UAbilitySystemComponent* InSourceASC,
		AActor* InTargetActor);

protected:
	/*~ APBProjectile Interface ~*/
	// Bezier 곡선 도착 시 데미지 적용, OnArrowResolved 브로드캐스트
	virtual void OnArrived() override;

public:
	// 도착 시 브로드캐스트 (어빌리티 종료 트리거)
	FOnArrowResolved OnArrowResolved;

private:
	// 적용할 데미지 GE 스펙
	FGameplayEffectSpecHandle DamageSpecHandle;

	// 발사자 AbilitySystemComponent
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	// 타겟 액터 (이 액터에만 데미지 적용)
	TWeakObjectPtr<AActor> TargetActor;
};
