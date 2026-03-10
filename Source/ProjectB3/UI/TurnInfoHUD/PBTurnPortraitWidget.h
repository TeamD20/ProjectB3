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

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	UPBTurnPortraitViewModel* PortraitViewModel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Turn")
	class UPBPortraitBaseWidget* PortraitWidget;
};
