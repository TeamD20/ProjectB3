// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBPartyMemberViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPartyMemberSelectedSignature, AActor*);

using namespace PBUIDelegate;

/** 툴팁 리스트(상태, 대응 등)의 1줄을 담는 데이터 구조체 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBPartyTooltipRowData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip")
	FText Text;
};

/**
 * 
 */
UCLASS()
class PROJECTB3_API UPBPartyMemberViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	// 게터
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	FText GetCharacterName() const;
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	FText GetCharacterLevel() const;
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	FText GetCharacterClass() const;
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	FText GetCharacterHPText() const;
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	TSoftObjectPtr<UTexture2D> GetPortrait() const;
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	float GetHealthPercent() const;
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	int32 GetCurrentHP() const { return CurrentHP; }
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	int32 GetMaxHP() const { return MaxHP; }
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	bool bIsCharacterSelect() const;
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	const TArray<FPBPartyTooltipRowData>& GetStatusEffects() const { return StatusEffects; }

	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	const TArray<FPBPartyTooltipRowData>& GetReactions() const { return Reactions; }
	
public:
	// 세터
	void SetCharacterName(FText InCharacterName);
	void SetLevel(int32 InCharacterLevel);
	void SetCharacterClass(FText InCharacterClass);
	void SetHP(int32 InCurrentHP, int32 InMaxHP);
	void SetPortrait(TSoftObjectPtr<UTexture2D> InPortrait);
	void SetIsSelectedCharacter(bool InMyTurn);
	void SetStatusEffects(const TArray<FPBPartyTooltipRowData>& InStatusEffects);
	void SetReactions(const TArray<FPBPartyTooltipRowData>& InReactions);
	
	UFUNCTION(BlueprintCallable, Category = "UI | ViewModel")
	void OnSelected();
	
private: 
	void SetHealthPercent(float InHealthPercent);
	
public:
	// 델리게이트 변수
	FOnTextChangedSignature OnNameChanged;
	FOnTextChangedSignature OnLevelChanged;
	FOnTextChangedSignature OnClassChanged;
	FOnTextChangedSignature OnHPChanged;
	FOnImageChangedSignature OnPortraitChanged;
	FOnFloatValueChangedSignature OnHPPercentValueChanged;
	FOnBoolValueChangedSignature OnIsMyTurnChanged;
	FOnPartyMemberSelectedSignature OnPartyMemberSelected;

	DECLARE_MULTICAST_DELEGATE(FOnStatusEffectsChangedSignature);
	FOnStatusEffectsChangedSignature OnStatusEffectsChanged;

	DECLARE_MULTICAST_DELEGATE(FOnReactionsChangedSignature);
	FOnReactionsChangedSignature OnReactionsChanged;
	
private:
	FText CharacterName;
	FText CharacterClass;
	
	//초상화 
	TSoftObjectPtr<UTexture2D> Portrait;
	
	int32 Level;
	int32 CurrentHP;
	int32 MaxHP;
	float HealthPercent;
	bool bIsCharSelect;

	TArray<FPBPartyTooltipRowData> StatusEffects;
	TArray<FPBPartyTooltipRowData> Reactions;
};
