// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBSkillBarTypes.h"
#include "PBSkillSlotWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UOverlay;
class UPBSkillBarViewModel;

/** 스킬바의 단일 슬롯 UI 위젯 */
UCLASS()
class PROJECTB3_API UPBSkillSlotWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 슬롯의 현재 데이터를 설정하고 UI를 갱신한다.
	void SetSlotData(const FPBSkillSlotData& InSlotData);

	// 슬롯의 탭/인덱스 정보를 설정한다.
	void SetSlotIndex(int32 InTabIndex, int32 InSlotIndex);

	// 슬롯 클릭 시 참조할 스킬바 ViewModel을 설정한다.
	void InitializeBinding(UPBSkillBarViewModel* InSkillBarViewModel);

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 슬롯 클릭 시 선택 캐릭터에 어빌리티 발동을 요청한다.
	UFUNCTION()
	void OnSlotClicked();

private:
	// 슬롯 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	// 쿨다운 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText;

	// 클릭 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

	// 쿨다운 오버레이
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> CooldownOverlay;

	// 현재 슬롯 데이터
	FPBSkillSlotData SlotData;

	// 현재 탭 인덱스
	int32 TabIndex = INDEX_NONE;

	// 현재 슬롯 인덱스
	int32 SlotIndex = INDEX_NONE;

	// 스킬바 ViewModel 참조
	TWeakObjectPtr<UPBSkillBarViewModel> SkillBarViewModel;
};
