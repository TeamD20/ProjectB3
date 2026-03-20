// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "ProjectB3/AbilitySystem/Payload/PBFloatingTextPayload.h"
#include "ProjectB3/UI/CombatLog/PBCombatLogTypes.h"
#include "ProjectB3/Combat/PBCombatTypes.h"
#include "ProjectB3/UI/Combat/PBActionIndicatorTypes.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBAbilitySystemUIBridge.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;
class UPBViewModelSubsystem;
class UPBAbilitySystemComponent;
class UPBCombatLogViewModel;
class UPBCombatStateTextWidget;
class UPBSkillNameFloatingWidget;
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

	// === CombatStats 바인딩 ===

	// Movement 또는 MaxMovement 변경 시 CombatStatsVM을 갱신
	void HandleMovementChanged(const FOnAttributeChangeData& Data);

	// ArmorClass 변경 시 CombatStatsVM을 갱신
	void HandleArmorClassChanged(const FOnAttributeChangeData& Data);

	// HitBonus 변경 시 CombatStatsVM을 갱신
	void HandleHitBonusChanged(const FOnAttributeChangeData& Data);

	// SpellSaveDCModifier 또는 ProficiencyBonus 변경 시 CombatStatsVM을 갱신
	void HandleSpellSaveDCChanged(const FOnAttributeChangeData& Data);

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

	// === 턴 진행 바인딩 ===

	// ASC의 OnProgressTurnCompleted 콜백을 구독
	void BindProgressTurnDelegate();

	// OnProgressTurnCompleted 콜백을 해제
	void UnbindProgressTurnDelegate();

	// 턴 진행 완료 시 SkillBarVM의 쿨다운 상태를 갱신
	void HandleProgressTurnCompleted();

	// === 전투 결과 바인딩 ===

	// PBASC의 GE 실행/태그 업데이트 콜백을 구독
	void BindCombatResultDelegates();

	// PBASC의 GE 실행/태그 업데이트 콜백을 해제
	void UnbindCombatResultDelegates();

	// GE 실행 결과를 해석하여 플로팅 텍스트 이벤트 전송
	void HandleGEExecuted(const FGameplayEffectSpec& Spec, const FGameplayAttribute& Attribute, float EffectiveValue);

	// 태그 변경 이벤트를 상태 플로팅 텍스트로 전송
	void HandleTagUpdated(const FGameplayTag& Tag, bool bTagExists);

	// 플로팅 텍스트 GameplayEvent 전송 헬퍼
	void SendFloatingTextEvent(
		EPBFloatingTextType Type,
		float Magnitude,
		const FGameplayTag& MetaTag = FGameplayTag(),
		const FText& TextOverride = FText::GetEmpty()) const;

	// 컴뱃 로그 엔트리 생성 및 ViewModel에 전달
	void SendCombatLogEntry(EPBCombatLogType InLogType, const FText& LogText) const;

	// 전투 상태 변경 구독 (브리지 내부 상태 정리용)
	void BindCombatStateDelegate();
	void UnbindCombatStateDelegate();
	void HandleCombatStateChanged(EPBCombatState NewState);

	// === 행동 인디케이터 UI ===

	// 행동 인디케이터 VM 갱신 
	void UpdateActionIndicator(EPBActionIndicatorType Type, const FText& Text);

	// 행동 인디케이터 초기화
	void ClearActionIndicator();


	// Owner 액터의 전투 표시 이름 반환
	FText GetTargetDisplayName() const;

	// GESpec의 EffectCauser에서 전투 표시 이름 반환
	FText GetSourceDisplayName(const FGameplayEffectSpec& Spec) const;


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

	// 턴 진행 콜백 핸들
	FDelegateHandle ProgressTurnHandle;

	// 전투 결과 콜백 핸들
	FDelegateHandle GEExecutedHandle;
	FDelegateHandle TagUpdatedHandle;

	// 전투 상태 변경 이벤트 핸들
	FDelegateHandle CombatStateHandle;
};
