// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBPartyMemberTooltipWidget.generated.h"

class UPBPartyMemberViewModel;
class UTextBlock;
class UWrapBox;
class UPBPartyMemberTooltipRowWidget;

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
	// 레벨 + 클래스를 하나의 텍스트 블록에 합쳐 표시
	void HandleLevelAndClassChanged();
	
	void HandleBuffsChanged();
	void HandleDebuffsChanged();
	void HandleClassIconChanged(TSoftObjectPtr<UTexture2D> InIcon);

public:
	// 캐릭터 이름
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CharacterNameTextBlock;

	// 레벨 + 클래스 합쳐 표시용 (예: "4레벨 클레릭")
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CharacterLevelTextBlock;

	// 직업 표시 ("Class. X")
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CharacterClassTextBlock;
	
	// 직업 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UImage> ClassIconImage;

	UPROPERTY(meta = (BindWidget))
	UWrapBox* BuffBox;

	UPROPERTY(meta = (BindWidget))
	UWrapBox* DeBuffBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI | Tooltip Widget")
	TSubclassOf<UPBPartyMemberTooltipRowWidget> RowWidgetClass;

protected:
	UPROPERTY()
	UPBPartyMemberViewModel* MemberViewModel;
};
