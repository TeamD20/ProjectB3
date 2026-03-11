// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBTurnOrderViewModel.h"
#include "PBTurnPortraitWidget.h"
#include "Components/HorizontalBox.h"
#include "PBTurnOrderInfoWidget.generated.h"

UCLASS(Abstract)
class PROJECTB3_API UPBTurnOrderInfoWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 외부(GameMode등)에서 ViewModel 주입
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void SetupViewModel(UPBTurnOrderViewModel* InViewModel);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// View Model의 OnTurnOrderListChanged 등에 바인딩할 함수
	UFUNCTION()
	void HandleTurnOrderListChanged();

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget), Category = "Turn")
	UHorizontalBox* TurnPortraitContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	TSubclassOf<UPBTurnPortraitWidget> PortraitWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	UPBTurnOrderViewModel* TurnOrderViewModel;
};
