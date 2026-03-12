// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBPartyMemberViewModel.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBPartyMemberWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTB3_API UPBPartyMemberWidget : public UPBWidgetBase
{
	GENERATED_BODY()
public:
	void InitializeWithViewModel(UPBPartyMemberViewModel* ViewModel);
	void RefreshUI();
	
	
protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeDestruct() override;
	
	/*~ UTesT_PartyMemberWidget Interface ~*/
	void HandleHPChanged(FText InCurrentHP);
	void HandleMaxHPChanged(FText InMaxHP);	

	
	void UpdataHPText();
	
	void HandleImageChanged(TSoftObjectPtr<UTexture2D> InPortrait);
	void HandleCharacterSelected(bool bInMyTurn);

public:
	// UPROPERTY(meta = (BindWidget))
	// UTextBlock* CharacterNameTextBlock;
	//
	// UPROPERTY(meta = (BindWidget))
	// UTextBlock* CharacterMaxHPTextBlock;
	//
	// UPROPERTY(meta = (BindWidget))
	// UTextBlock* CharacterLevelTextBlock;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CharacterHPTextBlock;
	
	// 체력바(데미지 바) 연결용
	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* DamageProgressBar;
	
	UPROPERTY(meta = (BindWidget))
	class UPBPortraitBaseWidget* PortraitWidget;
	
	UPROPERTY(meta = (BindWidget))
	UOverlay* SelectedOverlay;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ViewModel")
	UPBPartyMemberViewModel* MemberViewModel;

protected:
	void HandleHPPercentChanged(float InHealthPercent);
};
