// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBGameplayAbility_ShowFloatingText.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/UI/Combat/PBFloatingTextActor.h"
#include "ProjectB3/UI/Combat/PBFloatingTextWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBFloatingText, Log, All);

UPBGameplayAbility_ShowFloatingText::UPBGameplayAbility_ShowFloatingText()
{
	// InstancedPerActor: 인스턴스가 액터 수명 동안 유지되어 ActiveWidgetCount 추적 가능
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Event.UI.FloatingText 이벤트로 트리거
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = PBGameplayTags::Event_UI_FloatingText;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UPBGameplayAbility_ShowFloatingText::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 베이스 ActivateAbility 호출 (턴 자원 관련 로직 등)
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TriggerEventData || !TriggerEventData->OptionalObject)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UPBFloatingTextPayload* Payload = Cast<UPBFloatingTextPayload>(const_cast<UObject*>(TriggerEventData->OptionalObject.Get()));
	if (!IsValid(Payload))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 타입별 위젯 클래스 결정
	TSubclassOf<UPBFloatingTextWidget> WidgetClass = ResolveWidgetClass(Payload->FloatingTextType);

	// 스태킹 오프셋 계산
	const float OffsetZ = SpawnOffsetZ + (ActiveWidgetCount * StackSpacing);

	// 위젯 액터 스폰
	APBFloatingTextActor* WidgetActor = SpawnWidgetActor(Payload, WidgetClass, OffsetZ);
	if (IsValid(WidgetActor))
	{
		ActiveWidgetCount++;
		WidgetActor->OnDestroyed.AddDynamic(this, &UPBGameplayAbility_ShowFloatingText::OnWidgetActorDestroyed);
	}

	// Fire-and-Forget: 즉시 종료 (인스턴스는 유지됨)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

TSubclassOf<UPBFloatingTextWidget> UPBGameplayAbility_ShowFloatingText::ResolveWidgetClass(EPBFloatingTextType Type) const
{
	if (const TSubclassOf<UPBFloatingTextWidget>* Found = WidgetClassByType.Find(Type))
	{
		if (*Found)
		{
			return *Found;
		}
	}
	return DefaultWidgetClass;
}

APBFloatingTextActor* UPBGameplayAbility_ShowFloatingText::SpawnWidgetActor(
	UPBFloatingTextPayload* Payload, TSubclassOf<UPBFloatingTextWidget> WidgetClass, float OffsetZ)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor) || !WidgetActorClass)
	{
		UE_LOG(LogPBFloatingText, Warning, TEXT("SpawnWidgetActor 실패: AvatarActor 또는 WidgetActorClass가 유효하지 않음"));
		return nullptr;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	// 스폰 위치: 캐릭터 위치 + Z 오프셋
	FVector SpawnLocation = AvatarActor->GetActorLocation() + FVector(0.f, 0.f, OffsetZ);
	FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

	APBFloatingTextActor* WidgetActor = World->SpawnActorDeferred<APBFloatingTextActor>(
		WidgetActorClass, SpawnTransform);
	if (IsValid(WidgetActor))
	{
		WidgetActor->InitWithPayload(Payload, WidgetClass);
		WidgetActor->FinishSpawning(SpawnTransform);
	}

	return WidgetActor;
}

void UPBGameplayAbility_ShowFloatingText::OnWidgetActorDestroyed(AActor* DestroyedActor)
{
	ActiveWidgetCount = FMath::Max(0, ActiveWidgetCount - 1);
}
