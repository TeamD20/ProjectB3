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

	if (!DamageSpecHandle.IsValid())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

	if (!ProjectileClass)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
		return;
	}

	// 투사체 콜백에서 EndAbility 호출 시 사용할 핸들·정보 캐싱
	CachedHandle = GetCurrentAbilitySpecHandle();
	CachedActorInfo = *GetCurrentActorInfo();
	CachedActivationInfo = GetCurrentActivationInfo();

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
		EndAbility(CachedHandle, &CachedActorInfo, CachedActivationInfo, true, true);
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

	// Spec DynamicTags에서 Equipment.Slot 태그를 탐색하여 부착된 무기 액터 반환
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
	EndAbility(CachedHandle, &CachedActorInfo, CachedActivationInfo, true, false);
}
