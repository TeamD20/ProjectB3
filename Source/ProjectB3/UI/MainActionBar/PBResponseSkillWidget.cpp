// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "ProjectB3/UI/MainActionBar/PBResponseSkillWidget.h"
#include "ProjectB3/UI/SkillBar/PBSkillBarViewModel.h"
#include "ProjectB3/UI/SkillBar/PBSkillSlotWidget.h"
#include "Components/PanelWidget.h"

void UPBResponseSkillWidget::InitializeResponseArea(UPBSkillBarViewModel* InViewModel)
{
	if (SkillBarViewModel == InViewModel)
	{
		return;
	}

	if (SkillBarViewModel)
	{
		SkillBarViewModel->OnSlotsChanged.Remove(SlotsChangedHandle);
		SlotsChangedHandle.Reset();
	}

	SkillBarViewModel = InViewModel;

	if (SkillBarViewModel)
	{
		SlotsChangedHandle = SkillBarViewModel->OnSlotsChanged.AddUObject(this, &UPBResponseSkillWidget::HandleSlotsChanged);
		RefreshResponseSkills();
	}
}

void UPBResponseSkillWidget::NativeDestruct()
{
	InitializeResponseArea(nullptr);
	Super::NativeDestruct();
}

void UPBResponseSkillWidget::RefreshResponseSkills()
{
	if (!ResponseSkillContainer)
	{
		return;
	}

	// 기존 슬롯 제거
	ResponseSkillContainer->ClearChildren();

	if (!SkillBarViewModel)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("[ResponseSkillArea] SkillBarViewModel is NULL!"));
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (SkillBarViewModel->ResponseActions.Num() == 0)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Yellow, TEXT("[ResponseSkillArea] 0 ResponseActions in ViewModel. Hiding."));
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 대응 스킬이 있으면 표시
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (!SkillSlotWidgetClass)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, TEXT("[ERROR] ResponseSkillArea: SkillSlotWidgetClass is NULL! Please attach it in Blueprint Details panel."));
		UE_LOG(LogTemp, Error, TEXT("[PBResponseSkillWidget] SkillSlotWidgetClass is NULL. Cannot create dynamic slots."));
		return;
	}

	int32 AddedCount = 0;
	// 뷰모델의 ResponseActions 개수만큼 슬롯 생성 및 추가
	for (int32 i = 0; i < SkillBarViewModel->ResponseActions.Num(); ++i)
	{
		if (UPBSkillSlotWidget* SlotWidget = CreateWidget<UPBSkillSlotWidget>(this, SkillSlotWidgetClass))
		{
			// 3번 카테고리(Response)의 i번째 슬롯으로 연동
			SlotWidget->SetSlotIndex(3, i);
			SlotWidget->InitializeBinding(SkillBarViewModel);
			SlotWidget->SetSlotData(SkillBarViewModel->ResponseActions[i]);
			
			ResponseSkillContainer->AddChild(SlotWidget);
		}
	}
}

void UPBResponseSkillWidget::HandleSlotsChanged()
{
	RefreshResponseSkills();
}
