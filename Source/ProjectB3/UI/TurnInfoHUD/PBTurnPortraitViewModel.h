// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "PBTurnPortraitViewModel.generated.h"

// 턴 오더 인물 하단에 띄울 순수 아이콘 전용 데이터 구조체
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBTurnStatusIconData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn|Status")
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * 
 */
UCLASS(BlueprintType, meta = (DisplayName = "Turn Portrait ViewModel"))
class PROJECTB3_API UPBTurnPortraitViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	// Init parameters (Data from GameMode or Global ViewModel)
	void InitializeTurnPortrait(const FPBTurnOrderEntry& InEntry);

	// Getters
	UFUNCTION(BlueprintPure, Category = "Turn|Portrait")
	FText GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintPure, Category = "Turn|Portrait")
	TSoftObjectPtr<UTexture2D> GetPortrait() const { return Portrait; }

	UFUNCTION(BlueprintPure, Category = "Turn|Portrait")
	bool IsAlly() const { return bIsAlly; }

	UFUNCTION(BlueprintPure, Category = "Turn|Portrait")
	bool IsCurrentTurn() const { return bIsCurrentTurn; }

	UFUNCTION(BlueprintPure, Category = "Turn|Portrait")
	bool IsDead() const { return bIsDead; }

	// Setters
	UFUNCTION(BlueprintCallable, Category = "Turn|Portrait")
	void SetIsCurrentTurn(bool bInIsCurrentTurn);

	UFUNCTION(BlueprintCallable, Category = "Turn|Portrait")
	void SetIsDead(bool bInIsDead);

	UFUNCTION(BlueprintCallable, Category = "Turn|Portrait")
	const TArray<FPBTurnStatusIconData>& GetBuffs() const { return Buffs; }

	UFUNCTION(BlueprintCallable, Category = "Turn|Portrait")
	const TArray<FPBTurnStatusIconData>& GetDebuffs() const { return Debuffs; }

	void SetBuffs(const TArray<FPBTurnStatusIconData>& InBuffs);
	void SetDebuffs(const TArray<FPBTurnStatusIconData>& InDebuffs);

public:
	PBUIDelegate::FOnBoolValueChangedSignature OnCurrentTurnChanged;
	PBUIDelegate::FOnBoolValueChangedSignature OnDeathStateChanged;
	PBUIDelegate::FOnTextChangedSignature OnDisplayNameChanged;
	PBUIDelegate::FOnImageChangedSignature OnPortraitChanged;
	PBUIDelegate::FOnFloatValueChangedSignature OnHPPercentValueChanged;

	UFUNCTION(BlueprintPure, Category = "Turn|Portrait")
	float GetHealthPercent() const { return HealthPercent; }

	UFUNCTION(BlueprintCallable, Category = "Turn|Portrait")
	void SetHealthPercent(float bInHealthPercent);

	DECLARE_MULTICAST_DELEGATE(FOnStatusListChangedSignature);
	FOnStatusListChangedSignature OnBuffsChanged;
	FOnStatusListChangedSignature OnDebuffsChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn|Portrait")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn|Portrait")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn|Portrait")
	bool bIsAlly = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn|Portrait")
	bool bIsCurrentTurn = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn|Portrait")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn|Portrait")
	float HealthPercent = 0.0f;

	TArray<FPBTurnStatusIconData> Buffs;
	TArray<FPBTurnStatusIconData> Debuffs;
};
