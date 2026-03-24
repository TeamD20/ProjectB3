// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "ProjectB3/Utils/PBBlueprintTypes.h"
#include "PBGameplayAbility.generated.h"

class APBEquipmentActor;
class UPBAbilitySystemComponent;
class APBGameplayPlayerController;
class APBCharacterBase;
class UTexture2D;

// AI 스코어링에서 어빌리티의 효과 카테고리를 구분하는 열거형.
// EPBAbilityType(자원 소모 유형: Action/BonusAction)과는 별개.
// EvaluateActionScore / EvaluateHealScore 등에서 분기 기준으로 사용.
UENUM(BlueprintType)
enum class EPBAbilityCategory : uint8
{
	Attack,     // 적 대상 데미지 (기본값)
	Heal,       // 아군 대상 회복
	Buff,       // 아군 대상 강화
	Debuff,     // 적 대상 약화
	Control,     // 적 대상 행동 제한 (CC)
	None         // 기타 상태
};

// AI 스코어링에서 Buff/Debuff의 HP 자동 환산 시 수정 대상 전투 수치.
// StatModType != None이고 EstimatedHPValue == 0이면 Clearinghouse가 전투 상황 기반으로 자동 계산.
UENUM(BlueprintType)
enum class EPBStatModType : uint8
{
	None,           // 수동 EstimatedHPValue 사용 (기본값)
	ArmorClass,     // AC 수정 → 피격 확률 변동 (d20: ±1 = 5%)
	AttackBonus,    // 명중 보너스 수정 → 명중률 변동 (d20: ±1 = 5%)
	SaveDC,         // 내성 DC 수정 → 내성 성공률 변동 (d20: ±1 = 5%)
	DamageBonus,    // 데미지 보너스 수정 → 직접 피해량 변동
};

/** 프로젝트 전용 GameplayAbility 기반 클래스. 모든 어빌리티는 이 클래스를 상속해서 구현. */
UCLASS()
class PROJECTB3_API UPBGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPBGameplayAbility();
	
	/*~ UPBGameplayAbility Interfaces ~*/
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "ProjectB3", meta = (ExpandEnumAsExecs = "Result", DisplayName = "GetPBCharacter"))
	APBCharacterBase* K2_GetPBCharacter(EPBValidResult& Result) const;
	
	APBCharacterBase* GetPBCharacter() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "ProjectB3", meta = (ExpandEnumAsExecs = "Result", DisplayName = "GetPBCharacter"))
	APBGameplayPlayerController* K2_GetPBPlayerController(EPBValidResult& Result) const;
	
	APBGameplayPlayerController* GetPBPlayerController() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "ProjectB3")
	UPBAbilitySystemComponent* GetPBAbilitySystemComponent() const;
	
	UPBAbilitySystemComponent* GetPBAbilitySystemComponentFromActorInfo(const FGameplayAbilityActorInfo* ActorInfo) const;
	
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

	// AI 스코어링용 어빌리티 카테고리 조회
	UFUNCTION(BlueprintPure, Category = "Ability|AI")
	EPBAbilityCategory GetAbilityCategory() const { return AbilityCategory; }

	// 턴 기반 쿨다운 턴 수 조회 (0이면 쿨다운 없음)
	UFUNCTION(BlueprintPure, Category = "Ability|Definition")
	int32 GetCooldownTurns() const { return CooldownTurns; }

	// AI 스코어링용: Buff/Debuff/CC의 HP 환산 기대값 (디자이너 설정)
	// 동적 계산이 어려운 효과에 대한 고정 추정값
	UFUNCTION(BlueprintPure, Category = "Ability|AI")
	float GetEstimatedHPValue() const { return EstimatedHPValue; }

	// AI 스코어링용: 효과 지속 턴 수 (Buff/Debuff/CC)
	UFUNCTION(BlueprintPure, Category = "Ability|AI")
	int32 GetEffectDuration() const { return EffectDuration; }

	// AI 스코어링용: Buff/Debuff가 수정하는 전투 수치 유형
	UFUNCTION(BlueprintPure, Category = "Ability|AI")
	EPBStatModType GetStatModType() const { return StatModType; }

	// AI 스코어링용: 전투 수치 수정량 (예: AC+2 → 2.0)
	UFUNCTION(BlueprintPure, Category = "Ability|AI")
	float GetStatModDelta() const { return StatModDelta; }

	// 중복 체크용: 이 어빌리티의 GE가 타겟에 부여하는 효과 태그
	UFUNCTION(BlueprintPure, Category = "Ability|AI")
	FGameplayTag GetEffectGrantedTag() const { return EffectGrantedTag; }

	// 어빌리티 타입 반환. 활성화 중이면 Spec의 DynamicTags 포함, 아니면 AbilityTags만 조회.
	UFUNCTION(BlueprintPure, Category = "Ability|Definition", meta = (DisplayName = "GetAbilityType"))
	EPBAbilityType K2_GetAbilityType() const;
	
	EPBAbilityType GetAbilityType(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

	// ==== 주사위 굴림 (DiceSpec 기반) ====

	//BonusAttributeOverride 수정치 + 숙련 보너스 기반 명중 굴림 (d20 + HitBonus vs TargetAC)
	// TargetASC: 피격 대상 ASC
	UFUNCTION(BlueprintCallable,BlueprintPure = false, Category = "Ability|Dice")
	FPBHitRollResult RollHit(const UAbilitySystemComponent* InTargetASC) const;

	//AttackModifierAttributeOverride 수정치 기반 데미지 굴림 (DiceCount·DiceFaces 자동 적용)
	UFUNCTION(BlueprintCallable,BlueprintPure = false, Category = "Ability|Dice")
	FPBDamageRollResult RollDamage(bool bCritical) const;

	// BonusAttributeOverride 수정치 기반 주문 난이도(DC)로 내성 굴림 (d20 + SaveBonus vs SpellSaveDC)
	// TargetASC: 피주문자 ASC
	UFUNCTION(BlueprintCallable,BlueprintPure = false, Category = "Ability|Dice")
	FPBSavingThrowResult RollSavingThrow(const UAbilitySystemComponent* InTargetASC) const;

	// 명중 굴림과 데미지 굴림을 시행하고 DamageEffectSpec 반환
	UFUNCTION(BlueprintCallable,BlueprintPure = false, Category = "Ability|Dice")
	FGameplayEffectSpecHandle MakeDamageEffectSpecFromHitDamageRoll(const UAbilitySystemComponent* InTargetASC, FPBHitRollResult& OutHitRollResult,FPBDamageRollResult& OutDamageRollResult) const;
	
	// 내성 굴림과 데미지 굴림을 시행하고 DamageEffectSpec 반환
	UFUNCTION(BlueprintCallable,BlueprintPure = false, Category = "Ability|Dice")
	FGameplayEffectSpecHandle MakeDamageEffectSpecFromSavingThrowDamageRoll(const UAbilitySystemComponent* InTargetASC, FPBSavingThrowResult& OutSavingThrowResult,FPBDamageRollResult& OutDamageRollResult) const;

	// 힐 주사위 굴림 → HealEffectSpec 반환 (명중/세이빙 없음)
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Ability|Dice")
	FGameplayEffectSpecHandle MakeHealSpec(const UAbilitySystemComponent* InTargetASC) const;

	// AI 스코링용: 명중 굴림 기반 유효 기댓값 (AttackBonus·AttackModifier 자동 산출)
	// TargetASC: 피격 대상 ASC
	UFUNCTION(BlueprintPure, Category = "Ability|Dice")
	float GetExpectedHitDamage(const UAbilitySystemComponent* InTargetASC,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags) const;

	// AI 스코링용: 내성 굴림 기반 유효 기댓값 (SaveBonus·AttackModifier 자동 산출)
	// TargetASC: 피주문자 ASC
	UFUNCTION(BlueprintPure, Category = "Ability|Dice")
	float GetExpectedSavingThrowDamage(const UAbilitySystemComponent* InTargetASC,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags) const;

	// AI 스코링용: 판정 없는 직접 데미지 기댓값 (AttackModifier 자동 산출)
	UFUNCTION(BlueprintPure, Category = "Ability|Dice")
	float GetExpectedDirectDamage(
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags) const;

	// --- C++ 오버로드: CDO 안전 (AI 스코어링용) ---
	// SourceASC를 외부에서 주입하여 CDO에서도 호출 가능.
	// OutRawAvg: (선택) 명중 시 평균 데미지 — 확률 미반영, KillBonus 판정용.
	float GetExpectedHitDamage(
		const UAbilitySystemComponent* InSourceASC,
		const UAbilitySystemComponent* InTargetASC,
		const FGameplayTagContainer& SourceTags,
		const FGameplayTagContainer& TargetTags,
		float* OutRawAvg) const;

	float GetExpectedSavingThrowDamage(
		const UAbilitySystemComponent* InSourceASC,
		const UAbilitySystemComponent* InTargetASC,
		const FGameplayTagContainer& SourceTags,
		const FGameplayTagContainer& TargetTags,
		float* OutRawAvg) const;

	float GetExpectedDirectDamage(
		const UAbilitySystemComponent* InSourceASC,
		const FGameplayTagContainer& SourceTags,
		const FGameplayTagContainer& TargetTags,
		float* OutRawAvg) const;

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

	// DynamicTag에 장비 슬롯 태그가 있으면 해당 무기를 캐릭터에 자동 부착
	void TryAutoAttachEquipment(const FGameplayAbilitySpecHandle& Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

	// Source to Target 방향 라인 트레이스로 HitResult를 생성하여 EffectContext에 설정
	void SetTraceHitResultToContext(FGameplayEffectContextHandle& ContextHandle, const UAbilitySystemComponent* InTargetASC) const;

	// Payload에서 타겟 데이터 추출 유틸리티
	FPBAbilityTargetData ExtractTargetDataFromEvent(const FGameplayEventData& EventData) const;

protected:
	// AI 스코어링용 어빌리티 효과 카테고리 (Attack/Heal/Buff/Debuff/Control)
	// EPBAbilityType(자원 소모 유형)과는 별개의 분류 축
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AI")
	EPBAbilityCategory AbilityCategory = EPBAbilityCategory::Attack;

	// Buff/Debuff/CC의 HP 환산 기대값 (디자이너가 추정하여 설정)
	// 예: AC+2 버프 = 8.0, 적 AC-2 디버프 = 6.0, 1턴 스턴 = 10.0
	// Attack/Heal은 DiceSpec에서 자동 산출하므로 사용하지 않음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AI",
		meta = (ClampMin = "0.0", EditCondition = "AbilityCategory != EPBAbilityCategory::Attack && AbilityCategory != EPBAbilityCategory::Heal"))
	float EstimatedHPValue = 0.0f;

	// 효과 지속 턴 수 (Buff/Debuff/CC)
	// DurationFactor = min(EffectDuration, 3) / 3 으로 스코어링에 반영
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AI",
		meta = (ClampMin = "1", ClampMax = "10", EditCondition = "AbilityCategory != EPBAbilityCategory::Attack && AbilityCategory != EPBAbilityCategory::Heal"))
	int32 EffectDuration = 1;

	// (선택) Buff/Debuff가 수정하는 전투 수치 유형.
	// None이 아니고 EstimatedHPValue == 0이면 Clearinghouse가 전투 상황 기반 자동 HP 환산.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AI",
		meta = (EditCondition = "AbilityCategory != EPBAbilityCategory::Attack && AbilityCategory != EPBAbilityCategory::Heal"))
	EPBStatModType StatModType = EPBStatModType::None;

	// 전투 수치 수정량 (양수 = 강화/증가).
	// 예: Shield of Faith → StatModType=ArmorClass, StatModDelta=2.0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AI",
		meta = (EditCondition = "StatModType != EPBStatModType::None"))
	float StatModDelta = 0.0f;

	// 중복 체크용: 이 어빌리티의 GE가 타겟에 부여하는 GameplayTag.
	// 타겟이 이미 이 태그를 보유하면 AI가 해당 어빌리티를 스킵.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|AI",
		meta = (EditCondition = "AbilityCategory != EPBAbilityCategory::Attack && AbilityCategory != EPBAbilityCategory::None"))
	FGameplayTag EffectGrantedTag;

	// 턴 기반 쿨다운 턴 수 (0이면 쿨다운 없음)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Definition",
		meta = (ClampMin = "0", ClampMax = "20"))
	int32 CooldownTurns = 0;

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
	
	// 어빌리티 사용시 자동 장착할 액터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Equipment")
	TSoftClassPtr<APBEquipmentActor> EquipmentActorOverride;
	
	// 어빌리티 사용시 자동 장착할 슬롯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Equipment")
	FGameplayTag EquipmentSlotOverride = PBGameplayTags::Equipment_Slot_RightHand;
};
