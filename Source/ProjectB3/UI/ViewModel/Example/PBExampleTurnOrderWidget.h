// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBExampleTurnOrderWidget.generated.h"

class UPBExampleTurnOrderViewModel;

/**
 * TurnOrderViewModel을 사용하는 Slate 기반 위젯 예시.
 * RebuildWidget()에서 Slate로 UI를 구성하고,
 * NativeConstruct()에서 ViewModel 델리게이트에 바인딩한다.
 */
UCLASS(meta = (DisplayName = "Turn Order Widget (Example)"))
class PROJECTB3_API UPBExampleTurnOrderWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	UPBExampleTurnOrderWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// ViewModel 델리게이트 콜백
	UFUNCTION()
	void HandleTurnOrderChanged();

	UFUNCTION()
	void HandleTurnAdvanced(int32 NewIndex);

	// Slate UI 갱신
	void RebuildTurnList();
	
	// 턴 카드 생성 헬퍼
	TSharedRef<SWidget> MakeTurnCard(const struct FPBTurnOrderEntry& Entry, bool bIsCurrent);

private:
	// ViewModel 캐시 (NativeConstruct에서 획득)
	UPROPERTY()
	TObjectPtr<UPBExampleTurnOrderViewModel> CachedViewModel;
	
	// Slate 위젯 참조
	TSharedPtr<SHorizontalBox> TurnListBox;
	TSharedPtr<STextBlock> RoundText;
};
