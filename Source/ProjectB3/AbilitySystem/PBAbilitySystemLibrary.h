// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PBAbilityTypes.h"
#include "Attributes/PBCharacterAttributeSet.h"
#include "PBAbilitySystemLibrary.generated.h"

class UPBAbilitySystemComponent;
struct FPBAbilityGrantedHandles;
/** 어빌리티 관련 유틸리티 라이브러리 */
UCLASS()
class PROJECTB3_API UPBAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ==== 타겟 데이터 유틸리티 ====
	
	// 단일 타겟 액터 반환 (SingleTarget, Self 용). 유효하지 않으면 nullptr.
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static AActor* GetSingleTargetActor(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 전체 타겟 액터 배열 반환 (MultiTarget 용)
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static TArray<AActor*> GetAllTargetActors(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 단일 타겟 위치 반환. 없으면 ZeroVector.
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FVector GetSingleTargetLocation(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 전체 타겟 위치 배열 반환
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static TArray<FVector> GetAllTargetLocations(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 액터 또는 위치 타겟이 하나라도 존재하는지 확인
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static bool HasTarget(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 타겟팅 모드 기반 유효성 검증
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData", DisplayName = "Is Valid (Target Data)")
	static bool IsTargetDataValid(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// 전체 AoE 히트 액터 배열 반환
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static TArray<AActor*> GetAllHitActors(UPARAM(ref) const FPBAbilityTargetData& TargetData);

	// ==== 전투 굴림 유틸리티 ====

	// d20 명중 굴림 수행. Natural 20 = 치명타, Natural 1 = 자동 실패. 그 외 Roll + HitBonus >= TargetAC 이면 명중.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static FPBHitRollResult RollHit(int32 HitBonus, int32 TargetAC);

	// 무기 데미지 주사위 굴림. bCritical이면 주사위 수 2배. ExecCalc SetByCaller 전달용.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static FPBDamageRollResult RollDamage(int32 DiceCount, int32 DiceFaces, float AttackModifier, bool bCritical);

	// 내성 굴림 수행. D&D 5e: Natural 1/20 자동 실패/성공 없음. Roll + SaveBonus >= SpellSaveDC이면 성공.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static FPBSavingThrowResult RollSavingThrow(int32 SaveBonus, int32 SpellSaveDC);

	// 최종 데미지 계산 (ExecCalc + UI/AI 공용). SourceTags/TargetTags 기반 보정 추후 확장.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static float CalcFinalDamage(float DiceRoll, float AttackModifier,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags);

	// 평균 기댓값 기반 데미지 계산 (AI 행동 결정, UI 미리보기용).
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static float CalcExpectedDamage(int32 DiceCount, int32 DiceFaces, float AttackModifier,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags);

	// AI 스코링용: 명중 굴림 기반 유효 기댓값. P(일반명중) × 평균 데미지 + P(치명타) × 치명타 평균 데미지.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static float CalcExpectedAttackDamage(int32 DiceCount, int32 DiceFaces, float AttackModifier,
		int32 HitBonus, int32 TargetAC,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags);

	// AI 스코링용: 내성 굴림 기반 유효 기댓값. P(실패) × 전체 + P(성공) × 절반.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static float CalcExpectedSavingThrowDamage(int32 DiceCount, int32 DiceFaces, float AttackModifier,
		int32 SaveBonus, int32 SpellSaveDC,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags);

	// 힐 주사위 굴림. 회복량만 반환 (명중/세이빙 없음).
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static float RollHeal(int32 DiceCount, int32 DiceFaces);

	// 최종 회복량 계산. SourceTags/TargetTags 기반 보정 추후 확장.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static float CalcFinalHeal(float HealRoll,
		UPARAM(ref) const FGameplayTagContainer& SourceTags,
		UPARAM(ref) const FGameplayTagContainer& TargetTags);

	// 힐 GE 클래스 반환. 어빌리티에서 GE Spec 생성 시 사용.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static TSubclassOf<UGameplayEffect> GetHealGEClass(const UObject* WorldContextObject);

	// ==== 스탯 계산 유틸리티 ====

	// 능력치 수정치 계산: floor((AbilityScore - 10) / 2)
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static int32 CalcAbilityModifier(float AbilityScore);
	
	// 명중 수정치 반환 (BonusAttributeOverride 수정치만. 숙련 보너스 미포함).
	static int32 GetHitBonus(const UAbilitySystemComponent* ASC, const FGameplayAttribute& KeyAttributeOverride = FGameplayAttribute());
	
	// AttackModifier 반환 (AttackModifierAttributeOverride 수정치만. 숙련 보너스 미포함).
	static int32 GetAttackModifier(const UAbilitySystemComponent* ASC, const FGameplayAttribute& KeyAttributeOverride = FGameplayAttribute());
	
	// 주문 난이도(SpellSaveDC) 계산: 8 + ProficiencyBonus + AttributeOverride 수정치.
	// AttributeOverride 미지정 시 AttributeSet의 SpellSaveDCModifier 폴백 어트리뷰트 반환.
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static int32 CalcSpellSaveDC(const UAbilitySystemComponent* SourceASC, const FGameplayAttribute& KeyAttributeOverride = FGameplayAttribute());

	// 피주문자의 내성 보너스 계산: SaveAttribute 능력치 수정치. 미유효 시 0 반환.
	// SaveAttribute: DiceSpec.SaveAttributeOverride 전달.
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static int32 GetSaveBonus(const UAbilitySystemComponent* TargetASC, const FGameplayAttribute& SaveAttribute);

	// ASC에서 ProficiencyBonus 어트리뷰트 값을 정수로 반환. 조회 실패 시 0 반환.
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static int32 GetProficiencyBonus(const UAbilitySystemComponent* ASC);
	
	// 액터의 ASC에서 특정 어트리뷰트 수정치 가져오기. AttributeOverride 지정시 해당 Attribute로 계산하여 반환.
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static int32 GetAbilityModifierValue(const UAbilitySystemComponent* ASC, FGameplayAttribute Attribute, const FGameplayAttribute& KeyAttributeOverride = FGameplayAttribute() );
	
	// 액터의 ASC에서 특정 어트리뷰트 값 가져오기
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static float GetAttributeValue(AActor* Actor, FGameplayAttribute Attribute);
	
	// Constitution 기반 MaxHP 보너스 계산 (레벨 * Con Modifier)
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static float CalcMaxHPBonus(float Constitution, int32 Level);

	// Dexterity 기반 ArmorClass 계산 (BaseAC + Dex Modifier)
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static float CalcArmorClass(float Dexterity, float BaseAC = 10.f);

	// Dexterity 기반 Initiative 보너스 계산
	UFUNCTION(BlueprintPure, Category = "Ability|Stats")
	static float CalcInitiativeBonus(float Dexterity);

	// ==== 공용 전투 GE ====

	// 데미지 GE 클래스 반환. 어빌리티에서 GE Spec 생성 시 사용.
	UFUNCTION(BlueprintPure, Category = "Ability|Combat")
	static TSubclassOf<UGameplayEffect> GetDamageGEClass(const UObject* WorldContextObject);

	// DiceSpec.RollType에 따라 판정을 수행하고 SetByCaller가 완료된 데미지 GE Spec을 반환한다.
	// AttackRoll:   TargetASC의 ArmorClass로 명중 판정. 빗나가면 무효 핸들 반환.
	// SavingThrow: SourceASC의 SpellSaveDC vs TargetASC의 내성 보너스. 성공 시 절반 데미지.
	//             D&D 5e 규칙: 주문 데미지는 능력치 수정치 보너스 미적용.
	// None:        판정 없이 전체 데미지 Spec 반환.
	static FGameplayEffectSpecHandle MakeDamageEffectSpec(
		UAbilitySystemComponent* SourceASC,
		UAbilitySystemComponent* TargetASC,
		const FPBDiceSpec& DiceSpec);

	// ==== 캐릭터 어빌리티 시스템 초기화 ====
	UFUNCTION(Category = "Ability")
	static void ApplyStatsInitialization(UAbilitySystemComponent* ASC, FPBAbilityGrantedHandles& OutHandles, const FGameplayTag& CharacterTag, int32 CharacterLevel = 1);
	
	UFUNCTION(Category = "Ability")
	static void ApplyCommonAbilitySet(UPBAbilitySystemComponent* ASC, int32 CharacterLevel = 1);
	
	UFUNCTION(Category = "Ability")
	static void ApplyClassAbilitySet(UPBAbilitySystemComponent* ASC, const FGameplayTag& ClassTag, int32 CharacterLevel = 1);
};
