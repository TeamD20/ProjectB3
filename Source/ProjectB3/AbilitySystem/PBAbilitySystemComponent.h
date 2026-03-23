// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PBAbilityGrantTypes.h"
#include "PBAbilityTypes.h"
#include "PBAbilitySystemComponent.generated.h"

class UPBAbilitySetData;

DECLARE_LOG_CATEGORY_EXTERN(LogPBAbilitySystem, Log, All);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameplayTagUpdatedSignature, const FGameplayTag& /**Tag*/, bool /**TagExists*/)
DECLARE_MULTICAST_DELEGATE(FOnProgressTurnSignature);

// AttributeSet PostGameplayEffectExecute 결과 중계 델리게이트
// Spec: 태그(Miss/Save/Critical 등) 판별용, Attribute: 어트리뷰트 종류 식별, EffectiveValue: 실제 적용된 수치
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGEExecutedNotifySignature,
	const FGameplayEffectSpec& /*Spec*/,
	const FGameplayAttribute& /*Attribute*/,
	float /*EffectiveValue*/)

// 어빌리티 실행 시작 통지 델리게이트 — 전술 카메라 SkillFraming 진입 트리거
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnAbilityExecutionStartedSignature,
	const UGameplayAbility* /*Ability*/,
	const FPBAbilityTargetData& /*TargetData*/)

// 어빌리티 종료 통지 델리게이트 — Handle + bWasCancelled 포함 (UseConsumable 콜백용)
// GAS 기본 OnAbilityEnded는 bWasCancelled를 전달하지 않으므로 프로젝트 전용 확장
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPBAbilityEndedSignature,
	FGameplayAbilitySpecHandle /*Handle*/,
	bool                       /*bWasCancelled*/)

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTB3_API UPBAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UPBAbilitySystemComponent();
	
	// DA 기반 어빌리티 일괄 부여.
	void GrantAbilitiesFromData(
		const FGameplayTag& SourceTag,
		const UPBAbilitySetData* Data,
		int32 CharacterLevel = 1);

	// 특정 출처의 어빌리티 일괄 제거
	void RemoveAbilitiesBySource(const FGameplayTag& SourceTag);

	// GrantedHandleMap에 등록된 전체 어빌리티 제거
	void RemoveAllSourceGrantedAbilities();

	// 특정 출처에 부여된 핸들이 있는지 확인
	bool HasAbilitiesFromSource(const FGameplayTag& SourceTag) const;

	// 포함/제외 태그 조건으로 활성화 가능한 어빌리티 Spec 핸들 조회
	TArray<FGameplayAbilitySpecHandle> GetAbilitySpecHandlesByTagFilter(
		const FGameplayTagContainer& RequireTags,
		const FGameplayTagContainer& IgnoreTags = FGameplayTagContainer()) const;

	// Handle 기반으로 해당 어빌리티의 활성화 가능 여부를 조회한다.
	bool CanActivateAbilityByHandle(const FGameplayAbilitySpecHandle& Handle) const;

	// 턴 기반 어빌리티(EPBAbilityType != None) 실행 중 플래그 설정
	void SetTurnAbilityActive(bool bActive);

	// 턴 기반 어빌리티가 현재 실행 중인지 확인
	bool IsTurnAbilityActive() const { return bIsTurnAbilityActive; }

	// 이동 자원 최대치로 초기화
	void ResetMovementResource();
	
	// 한 턴 진행 후 호출, 적용된 이펙트들의 턴 스택 차감 및 쿨다운 감소
	void OnProgressTurn();

	// 해당 어빌리티가 쿨다운 중인지 확인
	bool HasCooldown(const FGameplayAbilitySpecHandle& Handle) const;

	// 어빌리티에 턴 기반 쿨다운 적용
	void ApplyCooldown(const FGameplayAbilitySpecHandle& Handle, int32 CooldownTurns);

	// 해당 어빌리티의 잔여 쿨다운 턴 수 반환 (없으면 0)
	int32 GetRemainingCooldown(const FGameplayAbilitySpecHandle& Handle) const;

	// AttributeSet::PostGameplayEffectExecute에서 호출
	void NotifyGEExecuted(const FGameplayEffectSpec& Spec, const FGameplayAttribute& Attribute, float EffectiveValue);

	// 어빌리티 실행 시작 통지 (K2_ExecuteTargetLogic에서 호출)
	void NotifyAbilityExecution(const UGameplayAbility* Ability, const FPBAbilityTargetData& TargetData);

	// UPBGameplayAbility::EndAbility에서 호출 — Handle + bWasCancelled 포함 종료 통지
	void NotifyPBAbilityEnded(FGameplayAbilitySpecHandle Handle, bool bWasCancelled);
	
protected:
	/*~ UActorComponent Interface ~*/
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/*~ UAbilitySystemComponent Interface ~*/
	virtual void OnTagUpdated(const FGameplayTag& Tag, bool TagExists) override;
	
private:
	void HandleActiveTurnChanged(AActor* CurrentTurnActor, int32 TurnIndex);
	
public:
	FOnGameplayTagUpdatedSignature OnGameplayTagUpdated;

	// 턴 진행 완료 시 브로드캐스트 (쿨다운·이펙트 차감 후)
	FOnProgressTurnSignature OnProgressTurnCompleted;

	// GE 실행 결과 알림 (AttributeSet의 PostGameplayEffectExecuted 처리 결과)
	FOnGEExecutedNotifySignature OnGEExecuted;

	// 어빌리티 실행 시작 알림 (전술 카메라 SkillFraming 진입용)
	FOnAbilityExecutionStartedSignature OnAbilityExecutionStarted;

	// 어빌리티 종료 알림 — Handle + bWasCancelled 포함 (UseConsumable 등 감시용)
	FOnPBAbilityEndedSignature OnPBAbilityEnded;
	
protected:
	// 출처별 핸들 캐시
	TMap<FGameplayTag, FPBAbilityGrantedHandles> GrantedHandleMap;

private:
	// 턴 기반 어빌리티(EPBAbilityType != None) 실행 중 플래그
	bool bIsTurnAbilityActive = false;

	// 어빌리티별 잔여 쿨다운 턴 수
	TMap<FGameplayAbilitySpecHandle, int32> CooldownMap;
};
