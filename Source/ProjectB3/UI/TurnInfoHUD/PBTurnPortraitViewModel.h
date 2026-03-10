// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "PBTurnPortraitViewModel.generated.h"

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

public:
	// Delegates
	PBUIDelegate::FOnBoolValueChangedSignature OnCurrentTurnChanged;
	PBUIDelegate::FOnBoolValueChangedSignature OnDeathStateChanged;
	PBUIDelegate::FOnTextChangedSignature OnDisplayNameChanged;
	PBUIDelegate::FOnImageChangedSignature OnPortraitChanged;

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
};
