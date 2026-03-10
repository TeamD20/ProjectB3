// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBGameplayAbility.generated.h"

class UTexture2D;

/** 프로젝트 전용 GameplayAbility 기반 클래스. 모든 어빌리티는 이 클래스를 상속해서 구현. */
UCLASS()
class PROJECTB3_API UPBGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	// UI 표시용 어빌리티 이름 반환
	const FText& GetAbilityDisplayName() const { return AbilityDisplayName; }

	// UI 표시용 어빌리티 아이콘 반환
	TSoftObjectPtr<UTexture2D> GetAbilityIcon() const { return AbilityIcon; }

	// UI 표시용 어빌리티 설명 반환
	const FText& GetAbilityDescription() const { return AbilityDescription; }

	// GAS 내장 쿨다운 GE 미사용. 턴제 쿨다운은 별도 관리.
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	// 어빌리티 타입 반환. 활성화 중이면 Spec의 DynamicTags 포함, 아니면 AbilityTags만 조회.
	EPBAbilityType GetAbilityType() const;

	/*~ UGameplayAbility Interface ~*/
	// 타입 지정 어빌리티 중복 실행 방지 검사
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 타입 지정 어빌리티 실행 시 ASC 플래그 설정
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 타입 지정 어빌리티 종료 시 ASC 플래그 해제
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	// UI 표시용 어빌리티 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|UI")
	FText AbilityDisplayName;

	// UI 표시용 어빌리티 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|UI")
	TSoftObjectPtr<UTexture2D> AbilityIcon;

	// UI 표시용 어빌리티 설명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|UI")
	FText AbilityDescription;

	// Payload에서 타겟 데이터 추출 유틸리티
	FPBAbilityTargetData ExtractTargetDataFromEvent(const FGameplayEventData& EventData) const;

};
