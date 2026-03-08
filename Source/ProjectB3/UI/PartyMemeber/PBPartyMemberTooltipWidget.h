// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBPartyMemberTooltipWidget.generated.h"

class UPBPartyMemberViewModel;
class UTextBlock;

/**
 * Tooltip widget to display additional party member info.
 */
UCLASS()
class PROJECTB3_API UPBPartyMemberTooltipWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI | Tooltip")
	void InitializeTooltip(UPBPartyMemberViewModel* ViewModel);

protected:
	void RefreshUI();
	
	/*~ UUserWidget Interface ~*/
	virtual void NativeDestruct() override;

	void HandleNameChanged(FText InName);
	void HandleLevelChanged(FText InLevel);
	void HandleClassChanged(FText InClass);

public:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CharacterNameTextBlock;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CharacterLevelTextBlock;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CharacterClassTextBlock;

protected:
	UPROPERTY()
	UPBPartyMemberViewModel* MemberViewModel;
};
