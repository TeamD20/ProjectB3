// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBGameplayAbility.h"
#include "ProjectB3/AbilitySystem/Payload/PBFloatingTextPayload.h"
#include "PBGameplayAbility_ShowFloatingText.generated.h"

class APBFloatingTextActor;
class UPBFloatingTextWidget;

/**
 * Event.UI.FloatingText GameplayEvent에 의해 트리거되는 위젯 표시 전용 어빌리티.
 * InstancingPolicy = InstancedPerActor — 인스턴스가 액터 수명 동안 유지되므로
 * 활성 위젯 수를 추적하여 스태킹 오프셋을 조정할 수 있다.
 */
UCLASS()
class PROJECTB3_API UPBGameplayAbility_ShowFloatingText : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPBGameplayAbility_ShowFloatingText();

protected:
	/*~ UGameplayAbility Interface ~*/
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	// 타입별 위젯 클래스 해석
	TSubclassOf<UPBFloatingTextWidget> ResolveWidgetClass(EPBFloatingTextType Type) const;

	// 위젯 액터 스폰
	APBFloatingTextActor* SpawnWidgetActor(UPBFloatingTextPayload* Payload, TSubclassOf<UPBFloatingTextWidget> WidgetClass, float OffsetZ);

	// 위젯 액터 소멸 콜백
	UFUNCTION()
	void OnWidgetActorDestroyed(AActor* DestroyedActor);

protected:
	// 스폰할 위젯 액터 클래스
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	TSubclassOf<APBFloatingTextActor> WidgetActorClass;

	// 타입별 위젯 클래스 (BP에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	TMap<EPBFloatingTextType, TSubclassOf<UPBFloatingTextWidget>> WidgetClassByType;

	// 매핑 실패 시 폴백 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	TSubclassOf<UPBFloatingTextWidget> DefaultWidgetClass;

	// 캐릭터 머리 위 기본 오프셋
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float SpawnOffsetZ = 120.f;

	// 위젯 간 수직 간격
	UPROPERTY(EditDefaultsOnly, Category = "FloatingText")
	float StackSpacing = 30.f;

private:
	// 현재 활성 위젯 수 (인스턴스 멤버, End 후에도 유지)
	int32 ActiveWidgetCount = 0;
};
