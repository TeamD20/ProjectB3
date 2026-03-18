// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PBAbilitySystemRegistry.generated.h"

class UPBAbilitySetData;
class UGameplayEffect;
class UAbilitySystemComponent;
struct FPBAbilityGrantedHandles;
struct FPBGameplayTagDisplayRow;

// 어빌리티 관련 DA 중앙 매핑 레지스트리 (세이브/로드 등에서 사용)
UCLASS(BlueprintType)
class PROJECTB3_API UPBAbilitySystemRegistry : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 공용 AbilitySet 반환
	const UPBAbilitySetData* GetCommonAbilitySet() const;

	// 데미지 GE 클래스 반환
	TSubclassOf<UGameplayEffect> GetDamageGEClass() const { return GE_Damage; }

	// 태그로 DA를 동기 로드하여 반환 (미로드 시 LoadSynchronous)
	const UPBAbilitySetData* FindAbilitySetByTag(const FGameplayTag& Tag) const;

	// 태그로 DT Row를 조회하고 초기화 GE 3개를 순서대로 적용
	void ApplyStatsInitialization(UAbilitySystemComponent* ASC, FPBAbilityGrantedHandles& OutHandles, const FGameplayTag& CharacterTag, int32 CharacterLevel) const;

	// 태그에 해당하는 표시 이름 반환 (없으면 태그 문자열 반환)
	FText GetTagDisplayName(const FGameplayTag& Tag) const;

	// 태그에 해당하는 아이콘 반환 (없으면 null)
	TSoftObjectPtr<UTexture2D> GetTagIcon(const FGameplayTag& Tag) const;

	// 태그에 해당하는 표시 행 반환 (없으면 nullptr)
	const FPBGameplayTagDisplayRow* FindTagDisplayRow(const FGameplayTag& Tag) const;

protected:
	// 공용 어빌리티 세트
	UPROPERTY(EditDefaultsOnly, Category = "Registry")
	TSoftObjectPtr<UPBAbilitySetData> CommonAbilitySet;
	
	// 레지스트리 엔트리 (태그 : DA 소프트 참조)
	UPROPERTY(EditDefaultsOnly, Category = "Registry")
	TMap<FGameplayTag, TSoftObjectPtr<UPBAbilitySetData>> AbilitySetMap;

	// 태그 : DT Row 이름 매핑
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TMap<FGameplayTag, FName> GameplayTagRowMap;
	
	// ==== 스탯 초기화 공용 설정 ====

	// 캐릭터 스탯 DataTable (FPBCharacterStatsRow)
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSoftObjectPtr<UDataTable> CharacterStatsTable;

	// GameplayTag → 표시 이름/아이콘 DataTable (FPBGameplayTagDisplayRow)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftObjectPtr<UDataTable> GameplayTagDisplayTable;

	// Primary 능력치 초기화 GE (Instant, SetByCaller)
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSubclassOf<UGameplayEffect> GE_PrimaryAttributes;

	// Secondary 능력치 초기화 GE (Infinite)
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSubclassOf<UGameplayEffect> GE_SecondaryAttributes;

	// 최종 초기화 GE (Instant)
	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSubclassOf<UGameplayEffect> GE_InitializeVitals;
	
	// ==== 공용 전투 GE ====

	// 데미지 GE (Instant, PBEC_Damage ExecCalc). 모든 공격 어빌리티가 공유.
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<UGameplayEffect> GE_Damage;
};
