// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBSkillBarWidget.generated.h"

class UPanelWidget;
class UPBSkillBarViewModel;
struct FPBSkillSlotData;

/** 선택 파티원의 스킬 슬롯들을 표시하는 스킬바 위젯 */
UCLASS()
class PROJECTB3_API UPBSkillBarWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 모든 슬롯을 비우고 뷰모델 데이터에 맞춰 다시 생성한다.
	UFUNCTION(BlueprintCallable, Category = "UI|SkillBar")
	void RefreshSkillBar();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// -- [빨강 영역] 3개 카테고리 컨테이너 --
	// 주행동 슬롯 영역
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PrimaryActionContainer;

	// 보조행동 슬롯 영역
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SecondaryActionContainer;

	// 마법/주문 슬롯 영역
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SpellActionContainer;

	// 개별 스킬 슬롯 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|SkillBar", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UPBSkillSlotWidget> SkillSlotWidgetClass;

private:
	// 슬롯 전체 재구성 이벤트 핸들러
	void HandleSlotsChanged();

	// 개별 슬롯 갱신 이벤트 핸들러
	void HandleSlotUpdated(int32 CategoryIndex, int32 SlotIndex);

	// 스킬바 ViewModel 참조
	UPROPERTY(Transient)
	TObjectPtr<UPBSkillBarViewModel> SkillBarViewModel;

	// SlotsChanged 델리게이트 핸들
	FDelegateHandle SlotsChangedHandle;

	// SlotUpdated 델리게이트 핸들
	FDelegateHandle SlotUpdatedHandle;
};
