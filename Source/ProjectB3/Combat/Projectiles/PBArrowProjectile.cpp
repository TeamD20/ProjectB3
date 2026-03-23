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

void APBArrowProjectile::OnArrived()
{
	// 타겟에 데미지 적용
	if (TargetActor.IsValid() && SourceASC.IsValid() && DamageSpecHandle.IsValid())
	{
		if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(TargetActor.Get()))
		{
			UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent();
			if (IsValid(TargetASC))
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), TargetASC);
			}
		}
	}

	OnArrowResolved.ExecuteIfBound(TargetActor.Get());
	Destroy();
}
