// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBArrowProjectile.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

void APBArrowProjectile::SetupArrow(
	const FGameplayEffectSpecHandle& InDamageSpec,
	UAbilitySystemComponent* InSourceASC,
	AActor* InTargetActor)
{
	DamageSpecHandle = InDamageSpec;
	SourceASC = InSourceASC;
	TargetActor = InTargetActor;
}

void APBArrowProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 지정된 대상에 명중한 경우에만 데미지 이펙트 적용
	if (OtherActor == TargetActor.Get() && SourceASC.IsValid() && DamageSpecHandle.IsValid())
	{
		if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(OtherActor))
		{
			UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent();
			if (IsValid(TargetASC))
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
			}
		}
	}

	// 대상 명중·장애물 충돌 모두 어빌리티에 알리고 소멸
	OnArrowResolved.ExecuteIfBound(OtherActor);
	Destroy();
}

void APBArrowProjectile::OnRangeExceeded()
{
	// 사거리 초과 — 데미지 미적용, 어빌리티 종료 트리거
	OnArrowResolved.ExecuteIfBound(nullptr);
	Destroy();
}
