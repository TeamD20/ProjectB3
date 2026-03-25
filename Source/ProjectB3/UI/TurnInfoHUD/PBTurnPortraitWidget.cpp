// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBTurnPortraitWidget.h"
#include "PBTurnPortraitViewModel.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"

void UPBTurnPortraitWidget::SetPortraitViewModel(UPBTurnPortraitViewModel* InViewModel)
{
	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.RemoveAll(this);
		PortraitViewModel->OnDeathStateChanged.RemoveAll(this);
		PortraitViewModel->OnDisplayNameChanged.RemoveAll(this);
		PortraitViewModel->OnPortraitChanged.RemoveAll(this);
		PortraitViewModel->OnHPPercentValueChanged.RemoveAll(this);
		PortraitViewModel->OnBuffsChanged.RemoveAll(this);
		PortraitViewModel->OnDebuffsChanged.RemoveAll(this);
	}

	PortraitViewModel = InViewModel;

	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleCurrentTurnChanged);
		PortraitViewModel->OnDeathStateChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleDeathStateChanged);
		PortraitViewModel->OnDisplayNameChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleDisplayNameChanged);
		PortraitViewModel->OnPortraitChanged.AddUObject(this, &UPBTurnPortraitWidget::HandlePortraitChanged);
		PortraitViewModel->OnHPPercentValueChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleHPPercentChanged);
		// 버프/디버프 둘 다 동일한 핸들러로 통합
		PortraitViewModel->OnBuffsChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleStatusChanged);
		PortraitViewModel->OnDebuffsChanged.AddUObject(this, &UPBTurnPortraitWidget::HandleStatusChanged);

		// 초기 동기화
		HandleDisplayNameChanged(PortraitViewModel->GetDisplayName());
		HandlePortraitChanged(PortraitViewModel->GetPortrait());
		HandleCurrentTurnChanged(PortraitViewModel->IsCurrentTurn());
		HandleDeathStateChanged(PortraitViewModel->IsDead());
		HandleHPPercentChanged(PortraitViewModel->GetHealthPercent());
		HandleStatusChanged();
		
		bool bIsAlly = PortraitViewModel->IsAlly();
		BP_OnInitAllyState(bIsAlly);

		// 아군/적군 테두리 색상 처리
		if (OutlineImage)
		{
			// 블루프린트에서 설정한 아군/적군 테두리 색상 적용
			FLinearColor BorderColor = bIsAlly ? AllyBorderColor : EnemyBorderColor;
			OutlineImage->SetColorAndOpacity(BorderColor);
		}
	}
}

void UPBTurnPortraitWidget::NativeDestruct()
{
	if (PortraitViewModel)
	{
		PortraitViewModel->OnCurrentTurnChanged.RemoveAll(this);
		PortraitViewModel->OnDeathStateChanged.RemoveAll(this);
		PortraitViewModel->OnDisplayNameChanged.RemoveAll(this);
		PortraitViewModel->OnPortraitChanged.RemoveAll(this);
		PortraitViewModel->OnHPPercentValueChanged.RemoveAll(this);
		PortraitViewModel->OnBuffsChanged.RemoveAll(this);
		PortraitViewModel->OnDebuffsChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

#include "ProjectB3/UI/Common/PBPortraitBaseWidget.h"

void UPBTurnPortraitWidget::HandleCurrentTurnChanged(bool bIsCurrentTurn)
{
	BP_OnCurrentTurnChanged(bIsCurrentTurn);
}

void UPBTurnPortraitWidget::HandleDeathStateChanged(bool bIsDead)
{
	BP_OnDeathStateChanged(bIsDead);
}

void UPBTurnPortraitWidget::HandleDisplayNameChanged(FText NewName)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetDisplayName(NewName);
	}
}

void UPBTurnPortraitWidget::HandlePortraitChanged(TSoftObjectPtr<UTexture2D> NewPortrait)
{
	if (PortraitWidget)
	{
		PortraitWidget->SetPortraitImage(NewPortrait);
	}
}

void UPBTurnPortraitWidget::HandleHPPercentChanged(float InHealthPercent)
{
	if (DamageProgressBar)
	{
		DamageProgressBar->SetPercent(1.0f -InHealthPercent);
	}
}

void UPBTurnPortraitWidget::HandleStatusChanged()
{
	if (!StatusBox || !PortraitViewModel)
	{
		return;
	}

	StatusBox->ClearChildren();

	// 버프 아이콘 추가
	const TArray<FPBTurnStatusIconData>& Buffs = PortraitViewModel->GetBuffs();
	for (const FPBTurnStatusIconData& Row : Buffs)
	{
		UImage* IconImage = NewObject<UImage>(this);
		IconImage->SetBrushFromSoftTexture(Row.Icon);

		FSlateBrush Brush = IconImage->GetBrush();
		Brush.ImageSize = FVector2D(16.0f, 16.0f);
		IconImage->SetBrush(Brush);

		UWrapBoxSlot* WrapSlot = StatusBox->AddChildToWrapBox(IconImage);
		if (WrapSlot)
		{
			WrapSlot->SetPadding(FMargin(0.0f, 0.0f, 2.0f, 2.0f));
			WrapSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	// 디버프 아이콘 추가 (버프 뒤에 이어서 배치)
	const TArray<FPBTurnStatusIconData>& Debuffs = PortraitViewModel->GetDebuffs();
	for (const FPBTurnStatusIconData& Row : Debuffs)
	{
		UImage* IconImage = NewObject<UImage>(this);
		IconImage->SetBrushFromSoftTexture(Row.Icon);

		FSlateBrush Brush = IconImage->GetBrush();
		Brush.ImageSize = FVector2D(16.0f, 16.0f);
		IconImage->SetBrush(Brush);

		UWrapBoxSlot* WrapSlot = StatusBox->AddChildToWrapBox(IconImage);
		if (WrapSlot)
		{
			WrapSlot->SetPadding(FMargin(0.0f, 0.0f, 2.0f, 2.0f));
			WrapSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	StatusBox->SetVisibility((Buffs.Num() + Debuffs.Num()) > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
