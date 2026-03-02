// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBExampleCharacterStatViewModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, int32, NewHP, int32, MaxHP);

/**
 * 캐릭터 능력치 표시를 위한 Actor-Bound ViewModel 예시
 */
UCLASS(BlueprintType, meta = (DisplayName = "Character Stat ViewModel"))
class PROJECTB3_API UPBExampleCharacterStatViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	UPBExampleCharacterStatViewModel();

	virtual void InitializeForActor(AActor* InTargetActor, ULocalPlayer* InLocalPlayer) override;
	virtual void Deinitialize() override;

	// Setter (외부 시스템이 호출)
	UFUNCTION(BlueprintCallable, Category = "Character Stat")
	void SetCharacterName(const FText& InName);

	UFUNCTION(BlueprintCallable, Category = "Character Stat")
	void SetLevel(int32 InLevel);

	UFUNCTION(BlueprintCallable, Category = "Character Stat")
	void SetHP(int32 InHP, int32 InMaxHP);

	UFUNCTION(BlueprintCallable, Category = "Character Stat")
	void SetArmorClass(int32 InAC);

	UFUNCTION(BlueprintCallable, Category = "Character Stat")
	void SetAbilityScores(int32 InSTR, int32 InDEX, int32 InCON, int32 InINT, int32 InWIS, int32 InCHA);

	// Getter (Presentation Logic)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Stat|Formula")
	static int32 GetAbilityModifier(int32 Score);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Stat|Formula")
	int32 GetStrengthModifier() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Stat|Formula")
	static FText GetFormattedModifier(int32 Score);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Stat|Formula")
	int32 GetProficiencyBonus() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Stat|Formula")
	float GetHPRatio() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Stat|Formula")
	FText GetHPText() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Character Stat|Formula")
	bool IsBloodied() const;

	// Raw 프로퍼티 Getter
	FText GetCharacterName() const { return CharacterName; }
	int32 GetLevel() const { return Level; }
	int32 GetHP() const { return HP; }
	int32 GetMaxHP() const { return MaxHP; }
	int32 GetArmorClass() const { return ArmorClass; }
	int32 GetStrength() const { return Strength; }
	int32 GetDexterity() const { return Dexterity; }
	int32 GetConstitution() const { return Constitution; }
	int32 GetIntelligence() const { return Intelligence; }
	int32 GetWisdom() const { return Wisdom; }
	int32 GetCharisma() const { return Charisma; }

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Character Stat|Events")
	FOnStatChanged OnStatChanged;

	UPROPERTY(BlueprintAssignable, Category = "Character Stat|Events")
	FOnHPChanged OnHPChanged;

protected:
	UFUNCTION()
	void HandleActorDestroyed(AActor* DestroyedActor);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	FText CharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 HP = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 MaxHP = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 ArmorClass = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 Strength = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 Dexterity = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 Constitution = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 Intelligence = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 Wisdom = 10;

	UPROPERTY(BlueprintReadOnly, Category = "Character Stat")
	int32 Charisma = 10;
};
