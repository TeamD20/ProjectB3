// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBAbilitySystemRegistry.h"
#include "PBAbilitySetData.h"
#include "PBCharacterStatsRow.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "ProjectB3/AbilitySystem/PBAbilityGrantTypes.h"
#include "ProjectB3/PBGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBAbilitySetRegistry, Log, All);

const UPBAbilitySetData* UPBAbilitySystemRegistry::GetCommonAbilitySet() const
{
	return CommonAbilitySet.LoadSynchronous();
}

const UPBAbilitySetData* UPBAbilitySystemRegistry::FindAbilitySetByTag(const FGameplayTag& Tag) const
{
	const TSoftObjectPtr<UPBAbilitySetData>* Found = AbilitySetMap.Find(Tag);
	if (!Found)
	{
		UE_LOG(LogPBAbilitySetRegistry, Warning, TEXT("태그 [%s]에 매핑된 DA가 없습니다."), *Tag.ToString());
		return nullptr;
	}

	if (Found->IsNull())
	{
		UE_LOG(LogPBAbilitySetRegistry, Warning, TEXT("태그 [%s]의 DA 소프트 참조가 Null입니다."), *Tag.ToString());
		return nullptr;
	}

	// 미로드 시 동기 로드
	UPBAbilitySetData* LoadedDA = Found->LoadSynchronous();
	if (!IsValid(LoadedDA))
	{
		UE_LOG(LogPBAbilitySetRegistry, Error, TEXT("태그 [%s]의 DA 동기 로드에 실패했습니다."), *Tag.ToString());
		return nullptr;
	}

	return LoadedDA;
}

void UPBAbilitySystemRegistry::ApplyStatsInitialization(
	UAbilitySystemComponent* ASC,
	FPBAbilityGrantedHandles& OutHandles,
	const FGameplayTag& CharacterTag,
	int32 CharacterLevel) const
{
	// 태그로 Row 이름 조회
	const FName* FoundRowName = GameplayTagRowMap.Find(CharacterTag);
	if (!FoundRowName)
	{
		UE_LOG(LogPBAbilitySetRegistry, Verbose, TEXT("태그 [%s]에 매핑된 스탯 Row가 없습니다. 스탯 초기화 건너뜀."), *CharacterTag.ToString());
		return;
	}

	const FName StatsRowName = *FoundRowName;

	if (!GE_PrimaryAttributes)
	{
		UE_LOG(LogPBAbilitySetRegistry, Warning, TEXT("GE_PrimaryAttributes가 설정되지 않았습니다."));
		return;
	}

	// DataTable 로드
	UDataTable* StatsTable = CharacterStatsTable.LoadSynchronous();
	if (!IsValid(StatsTable))
	{
		UE_LOG(LogPBAbilitySetRegistry, Warning, TEXT("CharacterStatsTable이 유효하지 않습니다."));
		return;
	}

	// Row 조회
	const FPBCharacterStatsRow* Row = StatsTable->FindRow<FPBCharacterStatsRow>(StatsRowName, TEXT("ApplyStatsInitialization"));
	if (!Row)
	{
		UE_LOG(LogPBAbilitySetRegistry, Warning, TEXT("Row '%s'을(를) 찾을 수 없습니다."), *StatsRowName.ToString());
		return;
	}

	const FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();

	// 1단계: Primary 능력치 초기화 (Instant, SetByCaller)
	{
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GE_PrimaryAttributes, CharacterLevel, EffectContext);
		if (SpecHandle.IsValid())
		{
			FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
			Spec->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Attribute_Strength, Row->Strength);
			Spec->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Attribute_Dexterity, Row->Dexterity);
			Spec->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Attribute_Constitution, Row->Constitution);
			Spec->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Attribute_Intelligence, Row->Intelligence);
			Spec->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Attribute_MaxHP, Row->BaseMaxHP);
			Spec->SetSetByCallerMagnitude(PBGameplayTags::SetByCaller_Attribute_ArmorClass, Row->BaseArmorClass);

			ASC->ApplyGameplayEffectSpecToSelf(*Spec);

			UE_LOG(LogPBAbilitySetRegistry, Log, TEXT("Primary 스탯 초기화 완료 (Row: %s)"), *StatsRowName.ToString());
		}
	}

	// 2단계: Secondary 능력치 (Infinite)
	if (GE_SecondaryAttributes)
	{
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GE_SecondaryAttributes, CharacterLevel, EffectContext);
		if (SpecHandle.IsValid())
		{
			OutHandles.EffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()));

			UE_LOG(LogPBAbilitySetRegistry, Log, TEXT("Secondary 스탯 MMC 적용 완료"));
		}
	}

	// 3단계: 최종 초기화 (Instant, HP = MaxHP)
	if (GE_InitializeVitals)
	{
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GE_InitializeVitals, CharacterLevel, EffectContext);
		if (SpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

			UE_LOG(LogPBAbilitySetRegistry, Log, TEXT("최종 스탯 초기화 완료"));
		}
	}
}
