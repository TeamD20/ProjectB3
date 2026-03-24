// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility_RangedAttack.h"

#include "AbilitySystemComponent.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/ItemSystem/PBEquipmentActor.h"
#include "ProjectB3/PBGameplayTags.h"

void UPBGameplayAbility_RangedAttack::FireProjectile(const FGameplayEffectSpecHandle& DamageSpecHandle, const FVector& TargetLocation, AActor* TargetActor)
{
	// 시전자 캐릭터 획득
	APBCharacterBase* Character = GetPBCharacter();
	if (!IsValid(Character))
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

	if (!ProjectileClass)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}
	
	// 발사 기점 Transform 결정
	const FTransform LaunchTransform = GetProjectileLaunchTransform();

	// 투사체 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = Character;

	APBProjectile* Projectile = GetWorld()->SpawnActor<APBProjectile>(
		ProjectileClass, LaunchTransform, SpawnParams);

	if (!IsValid(Projectile))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 초기화 및 발사
	Projectile->InitProjectile(DamageSpecHandle, GetAbilitySystemComponentFromActorInfo(), TargetActor);
	Projectile->OnProjectileResolved.BindUObject(this, &UPBGameplayAbility_RangedAttack::OnProjectileResolved);
	Projectile->Launch(TargetLocation);
	// EndMode == Manual — 투사체 OnArrowResolved 콜백에서 EndAbility 호출
}

FPBTargetingRequest UPBGameplayAbility_RangedAttack::MakeTargetingRequest() const
{
	FPBTargetingRequest Request = Super::MakeTargetingRequest();

	// 투사체 경로 프리뷰 컨텍스트 세팅
	if (ProjectileClass)
	{
		const APBProjectile* CDO = ProjectileClass.GetDefaultObject();

		Request.ProjectileContext.bShowPath = true;
		Request.ProjectileContext.LaunchLocation = GetProjectileLaunchTransform().GetLocation();
		Request.ProjectileContext.ArcHeightRatio = CDO->GetArcHeightRatio();
		Request.ProjectileContext.MinArcHeight = CDO->GetMinArcHeight();
		Request.ProjectileContext.MaxArcHeight = CDO->GetMaxArcHeight();
	}

	return Request;
}

FVector UPBGameplayAbility_RangedAttack::GetProjectileLaunchLocation() const
{
	return GetProjectileLaunchTransform().GetLocation();
}

void UPBGameplayAbility_RangedAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ResolvedCount = 0;
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UPBGameplayAbility_RangedAttack::OnProjectileHit_Implementation(AActor* HitActor)
{
	
}

FTransform UPBGameplayAbility_RangedAttack::GetProjectileLaunchTransform() const
{
	APBEquipmentActor* WeaponActor = GetEquippedWeaponActor();
	if (IsValid(WeaponActor))
	{
		return WeaponActor->GetProjectileLaunchTransform();
	}

	APBCharacterBase* Character = GetPBCharacter();
	return IsValid(Character) ? Character->GetActorTransform() : FTransform();
}

APBEquipmentActor* UPBGameplayAbility_RangedAttack::GetEquippedWeaponActor() const
{
	APBCharacterBase* Character = GetPBCharacter();
	if (!IsValid(Character))
	{
		return nullptr;
	}
	
	// 1. Override 설정으로 검색
	if (EquipmentSlotOverride.IsValid())
	{
		if (APBEquipmentActor* Attached = Character->GetAttachedEquipment(EquipmentSlotOverride))
		{
			return Attached;
		}
	}

	// 2. Spec DynamicTags에서 Equipment.Slot 태그를 탐색하여 부착된 무기 액터 반환
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (!Spec)
	{
		return nullptr;
	}

	for (const FGameplayTag& Tag : Spec->GetDynamicSpecSourceTags())
	{
		if (Tag.MatchesTag(PBGameplayTags::Equipment_Slot))
		{
			return Character->GetAttachedEquipment(Tag);
		}
	}

	return nullptr;
}

void UPBGameplayAbility_RangedAttack::OnProjectileResolved(AActor* HitActor)
{
	ResolvedCount++;
	
	if (IsValid(HitActor))
	{
		OnProjectileHit(HitActor);
	}
	
	if (ResolvedCount >= ProjectileCount)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);	
	}
}
