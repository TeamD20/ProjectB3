// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PBTargetingCursorWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// 타겟팅 커서 위젯. MultiTarget 모드에서 Material 기반 원형 게이지를 표시한다.
UCLASS(Abstract, BlueprintType)
class PROJECTB3_API UPBTargetingCursorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 게이지 진행률 갱신 (0.0 ~ 1.0)
	void SetGaugeProgress(float Progress);

	// 선택 수/최대 수 기반 게이지 갱신 및 카운트 텍스트 표시
	void SetGaugeCount(int32 Current, int32 Max);

	// 게이지 초기화 (숨김 처리)
	void ResetGauge();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;

private:
	// MID 생성 및 GaugeImage에 적용
	void InitGaugeMaterial();

protected:
	// 게이지 이미지 (BP에서 바인딩, 선택적)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> GaugeImage;

	// 카운트 텍스트 "2/4" (BP에서 바인딩, 선택적)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	// 게이지 베이스 머티리얼 (BP에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cursor|Gauge")
	TObjectPtr<UMaterialInterface> GaugeMaterial;

private:
	// 런타임 생성 MID
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> GaugeMID;
};
