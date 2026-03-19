// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBPartyMemberTooltipRowWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/**
 * 파티 멤버 툴팁 내부의 '상태' 혹은 '대응' 리스트의 1줄(Row)을 담당하는 위젯입니다.
 */
UCLASS()
class PROJECTB3_API UPBPartyMemberTooltipRowWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 내부 데이터를 초기화합니다.
	UFUNCTION(BlueprintCallable, Category = "UI | Tooltip Row")
	void InitializeRowData(TSoftObjectPtr<UTexture2D> InIcon, const FText& InText);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RowIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RowText;
};
