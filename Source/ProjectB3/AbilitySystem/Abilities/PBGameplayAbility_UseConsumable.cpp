// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility_UseConsumable.h"
#include "AbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/PBAbilitySystemComponent.h"
#include "ProjectB3/AbilitySystem/Payload/PBConsumableUsePayload.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Data/PBConsumableDataAsset.h"
#include "ProjectB3/PBGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBUseConsumable, Log, All);

UPBGameplayAbility_UseConsumable::UPBGameplayAbility_UseConsumable()
{
	// GameplayEvent 트리거 등록 — Event.Item.UseConsumable 수신 시 활성화
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = PBGameplayTags::Event_Item_UseConsumable;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 동일 인스턴스를 재사용 — EndAbility 후 재활성화 가능 상태 유지
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 어빌리티 타입 없음 — 행동 자원 미소모 (서브 어빌리티가 자체 코스트 처리)
	// AbilityTags에 Ability.Type 태그 없음 → EPBAbilityType::None
}

void UPBGameplayAbility_UseConsumable::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 1. 페이로드 추출
	if (!TriggerEventData)
	{
		UE_LOG(LogPBUseConsumable, Warning, TEXT("UseConsumable: TriggerEventData가 없습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	const UPBConsumableUsePayload* Payload = Cast<UPBConsumableUsePayload>(TriggerEventData->OptionalObject);
	if (!IsValid(Payload))
	{
		UE_LOG(LogPBUseConsumable, Warning, TEXT("UseConsumable: UPBConsumableUsePayload 캐스트 실패."));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	if (!IsValid(Payload->ConsumableDataAsset) || !Payload->ConsumableDataAsset->ConsumableAbilityClass)
	{
		UE_LOG(LogPBUseConsumable, Warning, TEXT("UseConsumable: ConsumableDataAsset 또는 ConsumableAbilityClass가 유효하지 않습니다."));
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	UPBAbilitySystemComponent* PBASC = GetPBAbilitySystemComponentFromActorInfo(ActorInfo);
	if (!IsValid(PBASC))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
		return;
	}

	// 2. 상태 저장 후 다음 틱으로 발동 지연
	// HandleGameplayEvent가 AbilityScopeLock을 잡은 채로 ActivateAbility를 호출하므로
	// 이 함수가 반환된 뒤 락이 해제될 때까지 GiveAbility 호출을 미룬다
	PendingInstanceID = Payload->InstanceID;
	PendingSubAbilityClass = Payload->ConsumableDataAsset->ConsumableAbilityClass;

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ActivateSubAbilityNextTick);
	// UseConsumable은 active 상태 유지 — EndAbility는 ActivateSubAbilityNextTick에서 처리
}

void UPBGameplayAbility_UseConsumable::ActivateSubAbilityNextTick()
{
	const FGameplayAbilitySpecHandle CurrentHandle = GetCurrentAbilitySpecHandle();

	UPBAbilitySystemComponent* PBASC = GetPBAbilitySystemComponent();
	if (!IsValid(PBASC) || !PendingSubAbilityClass)
	{
		PendingInstanceID = FGuid();
		PendingSubAbilityClass = nullptr;
		EndAbility(CurrentHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
		return;
	}

	// AbilityScopeLock 해제 이후 — 스펙이 ActivatableAbilities.Items에 즉시 추가됨
	FGameplayAbilitySpec Spec = PBASC->BuildAbilitySpecFromClass(PendingSubAbilityClass, 1);
	PendingSubAbilityHandle = Spec.Handle;
	PendingSubAbilityClass = nullptr;

	PBASC->OnPBAbilityEnded.AddUObject(this, &ThisClass::OnSubAbilityEnded);

	const FGameplayAbilitySpecHandle ActivatedHandle = PBASC->GiveAbilityAndActivateOnce(Spec);
	if (!ActivatedHandle.IsValid())
	{
		UE_LOG(LogPBUseConsumable, Log, TEXT("UseConsumable: GiveAbilityAndActivateOnce 실패 (코스트 부족 등)."));
		PBASC->OnPBAbilityEnded.RemoveAll(this);
		PendingSubAbilityHandle = FGameplayAbilitySpecHandle();
		PendingInstanceID = FGuid();
		EndAbility(CurrentHandle, CurrentActorInfo, CurrentActivationInfo, false, true);
	}
	// 발동 성공 — OnSubAbilityEnded 콜백 대기
}

void UPBGameplayAbility_UseConsumable::OnSubAbilityEnded(
	FGameplayAbilitySpecHandle EndedHandle, bool bWasCancelled)
{
	// 감시 중인 핸들이 아니면 무시
	if (EndedHandle != PendingSubAbilityHandle)
	{
		return;
	}

	UPBAbilitySystemComponent* PBASC = GetPBAbilitySystemComponent();

	// 5. 스택 차감 (취소되지 않은 경우에만)
	if (!bWasCancelled)
	{
		APBCharacterBase* Character = GetPBCharacter();
		if (IsValid(Character))
		{
			UPBInventoryComponent* InventoryComp = Character->GetInventoryComponent();
			if (IsValid(InventoryComp))
			{
				InventoryComp->RemoveItem(PendingInstanceID, 1);
			}
		}
	}

	// 6. 정리 — 스펙은 GiveAbilityAndActivateOnce의 RemoveAfterActivation으로 자동 제거됨
	if (IsValid(PBASC))
	{
		PBASC->OnPBAbilityEnded.RemoveAll(this);
	}

	PendingSubAbilityHandle = FGameplayAbilitySpecHandle();
	PendingInstanceID = FGuid();

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		false,
		false);
}
