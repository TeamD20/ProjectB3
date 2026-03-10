// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBPartyMemberViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPartyMemberSelectedSignature, AActor*);

using namespace PBUIDelegate;
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
	bool bIsCharacterSelect() const;
	
public:
	// 세터
	void SetCharacterName(FText InCharacterName);
	void SetLevel(int32 InCharacterLevel);
	void SetCharacterClass(FText InCharacterClass);
	void SetHP(int32 InCurrentHP, int32 InMaxHP);
	void SetPortrait(TSoftObjectPtr<UTexture2D> InPortrait);
	void SetIsSelectedCharacter(bool InMyTurn);
	
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
};
