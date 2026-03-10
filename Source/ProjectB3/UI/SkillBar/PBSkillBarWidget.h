// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBSkillBarWidget.generated.h"

class UHorizontalBox;
class UWidgetSwitcher;
class UPBSkillBarViewModel;
class UPBSkillSlotWidget;
struct FPBSkillSlotData;

/** 선택 파티원의 스킬 슬롯들을 표시하는 스킬바 위젯 */
UCLASS()
class PROJECTB3_API UPBSkillBarWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 특정 탭을 표시하도록 전환한다.
	UFUNCTION(BlueprintCallable, Category = "UI|SkillBar")
	void SetCurrentTab(int32 InTabIndex);

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 슬롯 전체 재구성 이벤트 핸들러
	void HandleSlotsChanged();

	// 개별 슬롯 갱신 이벤트 핸들러
	void HandleSlotUpdated(int32 TabIndex, int32 SlotIndex);

	// 탭 컨테이너에 슬롯 위젯들을 재생성한다. MaxSlotCount만큼 생성하고 데이터가 없는 슬롯은 빈 상태로 남긴다.
	void RebuildSlots(UPanelWidget* Container, const TArray<FPBSkillSlotData>& Slots, int32 TabIndex, int32 MaxSlotCount);

	// 탭 인덱스에 해당하는 컨테이너를 반환한다.
	UPanelWidget* GetContainerByTab(int32 TabIndex) const;

private:
	// 주행동 슬롯 컨테이너
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ActionSlotContainer;

	// 보조행동 슬롯 컨테이너
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> BonusActionSlotContainer;

	// 주문 슬롯 컨테이너
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SpellSlotContainer;

	// 탭 전환 스위처
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	// 슬롯 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|SkillBar", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPBSkillSlotWidget> SkillSlotWidgetClass;

	// 주행동 탭 슬롯 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|SkillBar", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 ActionSlotCount = 8;

	// 보조행동 탭 슬롯 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|SkillBar", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 BonusActionSlotCount = 8;

	// 주문 탭 슬롯 수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|SkillBar", meta = (AllowPrivateAccess = "true", ClampMin = 1))
	int32 SpellSlotCount = 8;

	// 스킬바 ViewModel 참조
	UPROPERTY(Transient)
	TObjectPtr<UPBSkillBarViewModel> SkillBarViewModel;

	// SlotsChanged 델리게이트 핸들
	FDelegateHandle SlotsChangedHandle;

	// SlotUpdated 델리게이트 핸들
	FDelegateHandle SlotUpdatedHandle;
};
