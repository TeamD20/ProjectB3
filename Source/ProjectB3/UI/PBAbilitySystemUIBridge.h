// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpecHandle.h"
#include "PBAbilitySystemUIBridge.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UPBViewModelSubsystem;
struct FAbilityEndedData;
struct FOnAttributeChangeData;

// ASC의 Attribute 변경 및 어빌리티 상태를 ViewModel에 중개하는 브리지 컴포넌트.
UCLASS(ClassGroup=(UserInterface), meta=(BlueprintSpawnableComponent))
class PROJECTB3_API UPBAbilitySystemUIBridge : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBAbilitySystemUIBridge();

protected:
	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// === 유틸리티 ===

	// ViewModelSubsystem을 조회
	UPBViewModelSubsystem* GetViewModelSubsystem() const;

	// === Attribute 바인딩 ===

	// ASC의 Attribute 변경 델리게이트를 구독
	void BindAttributeDelegates();

	// Attribute 변경 델리게이트를 일괄 해제
	void ClearAttributeBindings();

	// 개별 Attribute 바인딩 등록 헬퍼.
	void BindAttributeDelegate(const FGameplayAttribute& Attribute, void (UPBAbilitySystemUIBridge::*Handler)(const FOnAttributeChangeData&));

	// HP 또는 MaxHP 변경 시 ViewModel을 갱신
	void HandleHPChanged(const FOnAttributeChangeData& Data);

	// === 어빌리티 상태 바인딩 ===

	// ASC의 어빌리티 활성화/종료 콜백을 구독
	void BindAbilityDelegates();

	// 어빌리티 활성화/종료 콜백을 해제
	void UnbindAbilityDelegates();

	// 어빌리티 활성화 시 SkillBarVM의 해당 슬롯 bIsActive를 갱신
	void HandleAbilityActivated(UGameplayAbility* Ability);

	// 어빌리티 종료 시 SkillBarVM의 해당 슬롯 bIsActive를 해제
	void HandleAbilityEnded(const FAbilityEndedData& AbilityEndedData);

	// SkillBarVM에서 AbilityHandle에 해당하는 슬롯을 찾아 bIsActive를 갱신
	void UpdateSkillSlotActiveState(FGameplayAbilitySpecHandle Handle, bool bIsActive);

private:
	// Attribute 바인딩 단위
	struct FAttributeBinding
	{
		// 바인딩된 Attribute
		FGameplayAttribute Attribute;

		// 델리게이트 핸들
		FDelegateHandle DelegateHandle;

		bool IsValid() const { return Attribute.IsValid() && DelegateHandle.IsValid(); }
	};

	// 캐싱된 ASC 참조
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	// Attribute 델리게이트 바인딩 목록
	TArray<FAttributeBinding> AttributeBindings;

	// 어빌리티 콜백 핸들
	FDelegateHandle AbilityActivatedHandle;
	FDelegateHandle AbilityEndedHandle;
};
