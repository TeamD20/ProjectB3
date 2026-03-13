// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBSkillBarTabWidget.generated.h"

class UPBSkillBarViewModel;
class UPBSkillSlotWidget;
class UPanelWidget;
struct FPBSkillSlotData;

/** 메인 스킬바의 개별 탭(일반, 직업 등) 레이아웃을 통째로 묶은 독립 위젯 */
UCLASS()
class PROJECTB3_API UPBSkillBarTabWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯들을 갱신하고 컨테이너 하위에 재생성합니다.
	void RebuildSlots(const TArray<FPBSkillSlotData>& Slots, UPBSkillBarViewModel* ViewModel, int32 InTabIndex, int32 MaxSlotCount, TSubclassOf<UPBSkillSlotWidget> SlotWidgetClass);

	// 개별 슬롯의 데이터를 부분 갱신합니다.
	void UpdateSlot(int32 SlotIndex, const FPBSkillSlotData& SlotData);

protected:
	// 스킬 슬롯 위젯들이 동적으로 생성되어 붙을 패널 (WrapBox 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotContainer;

private:
	int32 TabIndex = -1;
};
