// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBTurnOrderViewModel.h"
#include "PBTurnIndicatorWidget.generated.h"

UCLASS(Abstract)
class PROJECTB3_API UPBTurnIndicatorWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 외부(GameMode등)에서 ViewModel 주입 (이벤트 구독 용도)
	UFUNCTION(BlueprintCallable, Category = "Turn")
	void SetupViewModel(UPBTurnOrderViewModel* InViewModel);

protected:
	virtual void NativeDestruct() override;

	// OnTurnAdvanced 알림 수신
	UFUNCTION()
	void HandleTurnAdvanced(UPBTurnPortraitViewModel* NewTurnOwner);

	// 애니메이션 재생 및 UI 텍스트 업데이트 (BP에서 연출 구현)
	UFUNCTION(BlueprintImplementableEvent, Category = "Turn")
	void BP_PlayTurnIndicatorAnimation(const FText& OwnerName, bool bIsAlly);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Turn")
	UPBTurnOrderViewModel* TurnOrderViewModel;
};
