// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "PBResourceGroupWidget.generated.h"

class UPBResourceSlotWidget;
class UPanelWidget;
class UTexture2D;

/**
 * 특정 종류의 자원 슬롯들을 여러 개 동적으로 묶어 표시하는 그룹 위젯
 * 블루프린트에서 배경, 활성화 이미지, 슬롯 개수 등을 커스터마이징 가능
 */
UCLASS(Abstract)
class PROJECTB3_API UPBResourceGroupWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 자원 상태 동기화 (슬롯 재구성 및 상태 업데이트)
	UFUNCTION(BlueprintCallable, Category = "UI|Resource")
	void SetResourceCount(int32 CurrentCount, int32 MaxCount);

protected:
	virtual void NativePreConstruct() override;

	// 동적 생성된 슬롯들을 배치할 패널 (주로 HorizontalBox)
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional), Category = "UI|Resource")
	TObjectPtr<UPanelWidget> SlotContainer;

	// 에디터/초기화 시 표시할 기본 최대 자원 수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Resource|Appearance")
	int32 DefaultMaxCount = 3;

	// 동적으로 생성할 슬롯 위젯의 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Resource|Appearance")
	TSubclassOf<UPBResourceSlotWidget> SlotWidgetClass;

	// 슬롯에 적용할 배경 텍스처
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Resource|Appearance")
	TSoftObjectPtr<UTexture2D> BackgroundTexture;

	// 슬롯에 적용할 활성화 상태 텍스처
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Resource|Appearance")
	TSoftObjectPtr<UTexture2D> ActiveTexture;

private:
	// 생성된 슬롯 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPBResourceSlotWidget>> CreatedSlots;

	// 슬롯 재구성 로직
	void RebuildSlots(int32 MaxCount);
};
