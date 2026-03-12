// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PBGameInstance.generated.h"

struct FGameplayTag;
class UPBAbilitySetData;
class UPBAbilitySystemRegistry;

// 게임 전역 데이터 및 설정을 보관하는 GameInstance.
UCLASS()
class PROJECTB3_API UPBGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/*~ UGameInstance Interface ~*/
	virtual void Init() override;

	/*~ UPBGameInstance Interface ~*/

	// 소프트 레퍼런스로 지정된 모든 레지스트리를 동기 로드하고 TSet에 보관
	void LoadAllRegistries();
	
	// === 에셋 반환 헬퍼 === 
	static const UPBAbilitySystemRegistry* GetAbilitySystemRegistry(const UObject* WorldContextObject);
	static const UPBAbilitySetData* GetCommonAbilitySet(const UObject* WorldContextObject);
	static const UPBAbilitySetData* FindAbilitySetByTag(const UObject* WorldContextObject, const FGameplayTag& Tag);
	
protected:
	// ==== 소프트 레퍼런스 (BP에서 지정) ====

	// 어빌리티 셋 레지스트리 DA
	UPROPERTY(EditDefaultsOnly, Category = "Data|Registries")
	TSoftObjectPtr<UPBAbilitySystemRegistry> AbilitySetRegistry;

private:
	// 로드된 에셋 보관 (GC 방지용)
	UPROPERTY()
	TSet<TObjectPtr<UObject>> LoadedAssets;
};
