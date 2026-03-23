// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility_RangedAttack.h"

#include "AbilitySystemComponent.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/ItemSystem/PBEquipmentActor.h"
#include "ProjectB3/PBGameplayTags.h"

void UPBGameplayAbility_RangedAttack::FireArrow(const FGameplayEffectSpecHandle& DamageSpecHandle, const FVector& TargetLocation, AActor* TargetActor)
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

	// 화살 콜백에서 EndAbility 호출 시 사용할 핸들·정보 캐싱
	CachedHandle = GetCurrentAbilitySpecHandle();
	CachedActorInfo = *GetCurrentActorInfo();
	CachedActivationInfo = GetCurrentActivationInfo();

	// 발사 기점 Transform 결정 (무기 소켓 → 폴백: 캐릭터 위치)
	APBEquipmentActor* WeaponActor = GetEquippedWeaponActor();
	const FTransform LaunchTransform = IsValid(WeaponActor)
		? WeaponActor->GetProjectileLaunchTransform()
		: Character->GetActorTransform();

	// 발사 방향 계산 (발사 기점 → TargetLocation)
	const FVector LaunchDirection = (TargetLocation - LaunchTransform.GetLocation()).GetSafeNormal();

	// 최대 거리
	const float MaxRange = FVector::Dist2D(TargetLocation,LaunchTransform.GetLocation()) + 30.f;
	
	// 화살 투사체 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Instigator = Character;

	APBArrowProjectile* Arrow = GetWorld()->SpawnActor<APBArrowProjectile>(
		ProjectileClass, LaunchTransform, SpawnParams);

	if (!IsValid(Arrow))
	{
		EndAbility(CachedHandle, &CachedActorInfo, CachedActivationInfo, true, true);
		return;
	}

	// 화살 초기화 및 발사
	Arrow->SetupArrow(DamageSpecHandle, GetAbilitySystemComponentFromActorInfo(), TargetActor, MaxRange);
	Arrow->OnArrowResolved.BindUObject(this, &UPBGameplayAbility_RangedAttack::OnArrowResolved);
	Arrow->Launch(LaunchDirection);
	// EndMode == Manual — 화살 OnArrowResolved 콜백에서 EndAbility 호출
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

void UPBGameplayAbility_RangedAttack::OnArrowResolved(AActor* HitActor)
{
	EndAbility(CachedHandle, &CachedActorInfo, CachedActivationInfo, true, false);
}
