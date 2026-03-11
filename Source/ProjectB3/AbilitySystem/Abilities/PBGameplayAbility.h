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
	/*~ UPBGameplayAbility Interfaces ~*/
	// UI 표시용 어빌리티 이름 반환
	UFUNCTION(BlueprintPure, Category = "Ability|Definition")
	const FText& GetAbilityDisplayName() const { return AbilityDisplayName; }

	// UI 표시용 어빌리티 아이콘 반환
	UFUNCTION(BlueprintPure, Category = "Ability|Definition")
	TSoftObjectPtr<UTexture2D> GetAbilityIcon() const { return AbilityIcon; }

	// UI 표시용 어빌리티 설명 반환
	UFUNCTION(BlueprintPure, Category = "Ability|Definition")
	const FText& GetAbilityDescription() const { return AbilityDescription; }

	// 주사위 스펙 조회 (DiceCount, DiceFaces, RollType)
	UFUNCTION(BlueprintPure, Category = "Ability|Definition")
	const FPBDiceSpec& GetDiceSpec() const { return DiceSpec; }

	// 어빌리티 타입 반환. 활성화 중이면 Spec의 DynamicTags 포함, 아니면 AbilityTags만 조회.
	UFUNCTION(BlueprintPure, Category = "Ability|Definition")
	EPBAbilityType GetAbilityType() const;

	// ==== 주사위 굴림 (DiceSpec 기반) ====

	//KeyAttributeOverride 수정치 + 숙련 보너스 기반 명중 굴림 (d20 + HitBonus vs TargetAC)
	UFUNCTION(BlueprintCallable, Category = "Ability|Dice")
	FPBHitRollResult RollHit(const UAbilitySystemComponent* InSourceASC, const UAbilitySystemComponent* InTargetASC) const;

	//KeyAttributeOverride 수정치 기반 데미지 굴림 (DiceCount·DiceFaces 자동 적용)
	UFUNCTION(BlueprintCallable, Category = "Ability|Dice")
	FPBDamageRollResult RollDamage(const UAbilitySystemComponent* InSourceASC, bool bCritical) const;

	// KeyAttributeOverride 수정치 + 숙련 보너스 기반 내성 굴림 (d20 + SaveBonus vs SpellSaveDC)
	UFUNCTION(BlueprintCallable, Category = "Ability|Dice")
	FPBSavingThrowResult RollSavingThrow(const UAbilitySystemComponent* InSourceASC, const UAbilitySystemComponent* InTargetASC) const;

	// AI 스코링용: 명중 굴림 기반 유효 기댓값 (AttackBonus·AttackModifier 자동 산출)
	UFUNCTION(BlueprintPure, Category = "Ability|Dice")
	float GetExpectedHitDamage(int32 TargetAC,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags) const;

	// AI 스코링용: 내성 굴림 기반 유효 기댓값 (SaveBonus·AttackModifier 자동 산출)
	UFUNCTION(BlueprintPure, Category = "Ability|Dice")
	float GetExpectedSavingThrowDamage(int32 SpellSaveDC,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags) const;

	// AI 스코링용: 판정 없는 직접 데미지 기댓값 (AttackModifier 자동 산출)
	UFUNCTION(BlueprintPure, Category = "Ability|Dice")
	float GetExpectedDirectDamage(
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags) const;
	
protected:
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

	// GAS 내장 쿨다운 GE 미사용. 턴제 쿨다운은 별도 관리.
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/*~ UPBGameplayAbility Interfaces ~*/

	// Payload에서 타겟 데이터 추출 유틸리티
	FPBAbilityTargetData ExtractTargetDataFromEvent(const FGameplayEventData& EventData) const;

protected:
	// 주사위 설정 (주사위 수, 면 수, 판정 유형, 핵심 능력치).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Definition")
	FPBDiceSpec DiceSpec;

	// UI 표시용 어빌리티 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Definition")
	FText AbilityDisplayName;

	// UI 표시용 어빌리티 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Definition")
	TSoftObjectPtr<UTexture2D> AbilityIcon;

	// UI 표시용 어빌리티 설명
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Definition")
	FText AbilityDescription;
};
