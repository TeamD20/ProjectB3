// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "PBPortraitBaseWidget.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class PROJECTB3_API UPBPortraitBaseWidget : public UPBWidgetBase
{
	GENERATED_BODY()
	
public:
	// 위젯에 초상화 이미지 설정
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void SetPortraitImage(TSoftObjectPtr<UTexture2D> InPortrait);
	
	// 위젯에 캐릭터 이름 설정 (옵션)
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void SetDisplayName(const FText& InName);

	// 위젯에 체력 퍼센트 설정 (옵션)
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void SetHealthPercent(float InPercent);
	
	// 위젯에 체력 텍스트 설정 (옵션)
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void SetHealthText(const FText& InText);

protected:
	// 블루프린트에서 시각적 연출을 담당할 이벤트 (머티리얼 파라미터 셋업, 색상 전환 등)
	UFUNCTION(BlueprintImplementableEvent, Category = "Portrait")
	void BP_OnPortraitChanged(const TSoftObjectPtr<UTexture2D>& NewPortrait);

	UFUNCTION(BlueprintImplementableEvent, Category = "Portrait")
	void BP_OnDisplayNameChanged(const FText& NewName);

	UFUNCTION(BlueprintImplementableEvent, Category = "Portrait")
	void BP_OnHealthPercentChanged(float NewPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Portrait")
	void BP_OnHealthTextChanged(const FText& NewText);

protected:
	// Base 공통 컴포넌트들
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Portrait")
	UImage* PortraitImage;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Portrait")
	UTextBlock* DisplayNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Portrait")
	class UProgressBar* HealthProgressBar;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Portrait")
	UTextBlock* HealthText;
};
