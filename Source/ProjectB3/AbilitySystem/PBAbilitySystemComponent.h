// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PBAbilityGrantTypes.h"
#include "PBAbilitySystemComponent.generated.h"

class UPBAbilitySetData;

DECLARE_LOG_CATEGORY_EXTERN(LogPBAbilitySystem, Log, All);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameplayTagUpdatedSignature, const FGameplayTag& /**Tag*/, bool /**TagExists*/)
DECLARE_MULTICAST_DELEGATE(FOnProgressTurnSignature);

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
	
protected:
	// 출처별 핸들 캐시
	TMap<FGameplayTag, FPBAbilityGrantedHandles> GrantedHandleMap;

private:
	// 턴 기반 어빌리티(EPBAbilityType != None) 실행 중 플래그
	bool bIsTurnAbilityActive = false;

	// 어빌리티별 잔여 쿨다운 턴 수
	TMap<FGameplayAbilitySpecHandle, int32> CooldownMap;
};
