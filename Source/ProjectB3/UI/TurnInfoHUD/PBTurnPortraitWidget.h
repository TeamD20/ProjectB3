// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBTurnPortraitViewModel.h"
#include "PBTurnPortraitWidget.generated.h"

UCLASS(Abstract)
class PROJECTB3_API UPBTurnPortraitWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 생성 시 바인딩
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void SetPortraitViewModel(UPBTurnPortraitViewModel* InViewModel);

protected:
	virtual void NativeDestruct() override;

	// ViewModel 이벤트 콜백 (Blueprint에서 시각 요소 변경 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "Turn")
	void BP_OnInitAllyState(bool bIsAlly);

	UFUNCTION(BlueprintImplementableEvent, Category = "Turn")
	void BP_OnCurrentTurnChanged(bool bIsCurrentTurn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Turn")
	void BP_OnDeathStateChanged(bool bIsDead);

private:
	// 콜백 매핑
	void HandleCurrentTurnChanged(bool bIsCurrentTurn);
	void HandleDeathStateChanged(bool bIsDead);
	void HandleDisplayNameChanged(FText NewName);
	void HandlePortraitChanged(TSoftObjectPtr<UTexture2D> NewPortrait);
	void HandleHPPercentChanged(float InHealthPercent);
	void HandleBuffsChanged();
	void HandleDebuffsChanged();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	UPBTurnPortraitViewModel* PortraitViewModel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Turn")
	class UPBPortraitBaseWidget* PortraitWidget;

	// 체력바(데미지 바) 연결용
	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* DamageProgressBar;

	UPROPERTY(meta = (BindWidgetOptional))
	class UWrapBox* BuffBox;

	UPROPERTY(meta = (BindWidgetOptional))
	class UWrapBox* DebuffBox;

	// 적군/아군 테두리 표시용
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Turn|Portrait")
	class UImage* OutlineImage;

	// 아군 테두리 색상 (블루프린트에서 편집 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn|Portrait|Colors")
	FLinearColor AllyBorderColor = FLinearColor(0.0f, 0.2f, 1.0f, 1.0f);

	// 적군 테두리 색상 (블루프린트에서 편집 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Turn|Portrait|Colors")
	FLinearColor EnemyBorderColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
};
