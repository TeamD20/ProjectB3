// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBResourceSlotWidget.generated.h"

class UImage;

/**
 * 활성 여부에 따라 오버레이 이미지를 켜고 끄는 단일 자원 슬롯 위젯
 * 상호작용(버튼) 없이 시각적 상태만 표시합니다.
 */
UCLASS(Abstract)
class PROJECTB3_API UPBResourceSlotWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯의 활성/비활성 상태 갱신
	UFUNCTION(BlueprintCallable, Category = "UI|Resource")
	void SetIsActive(bool bIsActive);

	// 위젯 생성/갱신 시 배경과 활성 텍스처를 지정
	UFUNCTION(BlueprintCallable, Category = "UI|Resource")
	void SetTextures(TSoftObjectPtr<UTexture2D> InBackgroundTexture, TSoftObjectPtr<UTexture2D> InActiveTexture);

protected:
	virtual void NativePreConstruct() override;

	// 배경 텍스처를 표시할 이미지 위젯
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Resource")
	TObjectPtr<UImage> BackgroundImage;

	// 활성화되었을 때 표시할 이미지 위젯
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Resource")
	TObjectPtr<UImage> ActiveImage;

private:
	// 에디터 미리보기 등을 위한 내부 캐시
	UPROPERTY(Transient)
	TSoftObjectPtr<UTexture2D> DefaultBackgroundTexture;

	UPROPERTY(Transient)
	TSoftObjectPtr<UTexture2D> DefaultActiveTexture;
};
