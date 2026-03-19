// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillSlotWidget.h"
#include "PBSkillTooltipWidget.h"
#include "PBSkillBarViewModel.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/Border.h"

void UPBSkillSlotWidget::SetSlotData(const FPBSkillSlotData& InSlotData)
{
	SlotData = InSlotData;

	if (IsValid(IconImage))
	{
		if (!SlotData.Icon.IsNull())
		{
			IconImage->SetVisibility(ESlateVisibility::Visible);
			IconImage->SetBrushFromSoftTexture(SlotData.Icon);
		}
		else
		{
			// 아이콘이 없으면 숨겨서 빈 슬롯 배경만 보이게 함
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (IsValid(CooldownText))
	{
		const FText CooldownValueText = SlotData.CooldownRemaining > 0
			? FText::AsNumber(SlotData.CooldownRemaining)
			: FText::GetEmpty();
		CooldownText->SetText(CooldownValueText);
	}

	if (IsValid(CooldownOverlay))
	{
		const bool bShowOverlay = SlotData.CooldownRemaining > 0;
		CooldownOverlay->SetVisibility(bShowOverlay ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (IsValid(SlotButton))
	{
		SlotButton->SetIsEnabled(SlotData.bCanActivate && SlotData.CooldownRemaining <= 0);
	}

	// 포커스 테두리 가시성 설정
	if (IsValid(FocusBorder))
	{
		FocusBorder->SetVisibility(SlotData.bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	// 발동 불가 시각화
	if (IsValid(DisabledOverlay))
	{
		DisabledOverlay->SetVisibility(SlotData.bCanActivate ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void UPBSkillSlotWidget::SetSlotIndex(int32 InTabIndex, int32 InSlotIndex)
{
	TabIndex = InTabIndex;
	SlotIndex = InSlotIndex;
}

void UPBSkillSlotWidget::InitializeBinding(UPBSkillBarViewModel* InSkillBarViewModel)
{
	SkillBarViewModel = InSkillBarViewModel;
}

void UPBSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.AddDynamic(this, &UPBSkillSlotWidget::OnSlotClicked);
	}
}

void UPBSkillSlotWidget::NativeDestruct()
{
	if (IsValid(SlotButton))
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPBSkillSlotWidget::OnSlotClicked);
	}

	SkillBarViewModel.Reset();

	if (CachedToolTipWidget && CachedToolTipWidget->IsInViewport())
	{
		CachedToolTipWidget->RemoveFromParent();
	}

	Super::NativeDestruct();
}

void UPBSkillSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// 스킬이 할당된 경우에만 호버 이벤트를 발생시켜 툴팁을 띄울 수 있도록 함
	if (SlotData.AbilityHandle.IsValid() || !SlotData.Icon.IsNull())
	{
		OnSkillSlotHovered.Broadcast(SlotData, true);

		if (ToolTipWidgetClass)
		{
			if (!CachedToolTipWidget)
			{
				CachedToolTipWidget = CreateWidget<UPBSkillTooltipWidget>(GetWorld(), ToolTipWidgetClass);
				// 파티멤버 위젯처럼 마우스를 따라가는 UMG 기본 툴팁으로 설정
				SetToolTip(CachedToolTipWidget);
			}

			// 런타임 툴팁 데이터 주입
			CachedToolTipWidget->SetTooltipData(SlotData);
		}
	}
}

void UPBSkillSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (SlotData.AbilityHandle.IsValid() || !SlotData.Icon.IsNull())
	{
		OnSkillSlotHovered.Broadcast(SlotData, false);
	}
}

void UPBSkillSlotWidget::OnSlotClicked()
{
	if (!SkillBarViewModel.IsValid())
	{
		return;
	}

	FPBSkillSlotData CurrentData;
	if (!SkillBarViewModel->GetSlotData(TabIndex, SlotIndex, CurrentData))
	{
		return;
	}

	APBGameplayPlayerState* PlayerState = SkillBarViewModel->GetPlayerState();
	if (!IsValid(PlayerState))
	{
		return;
	}

	PlayerState->RequestAbilityActivation(CurrentData.AbilityHandle);
}
