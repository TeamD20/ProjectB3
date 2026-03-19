// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBSkillTooltipWidget.h"
#include "PBSkillBarTypes.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPBSkillTooltipWidget::SetTooltipData(const FPBSkillSlotData& InSlotData)
{
	// 아이콘
	if (SkillLargeIcon)
	{
		if (!InSlotData.Icon.IsNull())
		{
			SkillLargeIcon->SetBrushFromSoftTexture(InSlotData.Icon);
			SkillLargeIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	// 스킬 이름 
	if (SkillNameText)
	{
		SkillNameText->SetText(!InSlotData.DisplayName.IsEmpty() ? InSlotData.DisplayName : DummySkillName);
	}

	// 스킬 타입 / 클래스
	if (SkillTypeText)
	{
		SkillTypeText->SetText(!InSlotData.SkillType.IsEmpty() ? InSlotData.SkillType : DummySkillType);
	}

	// 스킬 설명
	if (SkillDescriptionText)
	{
		SkillDescriptionText->SetText(!InSlotData.Description.IsEmpty() ? InSlotData.Description : DummyDescription);
	}

	// 피해량 설명
	if (DamageDescText)
	{
		DamageDescText->SetText(!InSlotData.DamageDesc.IsEmpty() ? InSlotData.DamageDesc : DummyDamageDesc);
	}

	// 주사위 설명 & 아이콘
	if (DiceRollDescText)
	{
		if (!InSlotData.DiceRollDesc.IsEmpty())
		{
			DiceRollDescText->SetText(InSlotData.DiceRollDesc);
			DiceRollDescText->SetColorAndOpacity(InSlotData.DiceRollColor);
		}
		else
		{
			// 실 데이터가 없으면 더미 출력
			DiceRollDescText->SetText(DummyDiceRollDesc);
			DiceRollDescText->SetColorAndOpacity(DummyDiceRollColor);
		}
	}

	if (DiceRollIcon)
	{
		if (!InSlotData.DiceRollIcon.IsNull())
		{
			DiceRollIcon->SetBrushFromSoftTexture(InSlotData.DiceRollIcon);
			DiceRollIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else if (DummyDiceRollIcon)
		{
			DiceRollIcon->SetBrushFromTexture(DummyDiceRollIcon);
			DiceRollIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DiceRollIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 6. 사거리 (Action Range)
	if (ActionRangeText)
	{
		ActionRangeText->SetText(!InSlotData.ActionRange.IsEmpty() ? InSlotData.ActionRange : DummyActionRange);
	}

	// 7. 굴림 유형 (Roll Type)
	if (RollTypeText)
	{
		RollTypeText->SetText(!InSlotData.RollType.IsEmpty() ? InSlotData.RollType : DummyRollType);
	}

	// 8. 굴림 유형 아이콘 (Roll Type Icon)
	if (RollTypeIcon)
	{
		bool bFoundIcon = false;
		
		// 맵에 래핑되어 있는 해당 굴림 종류(Enum)의 아이콘 텍스쳐 시그니처가 존재할 시
		if (TSoftObjectPtr<UTexture2D>* MappedIconPtr = RollTypeIconMap.Find(InSlotData.RollTypeEnum))
		{
			if (!MappedIconPtr->IsNull())
			{
				RollTypeIcon->SetBrushFromSoftTexture(*MappedIconPtr);
				RollTypeIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				bFoundIcon = true;
			}
		}

		if (!bFoundIcon)
		{
			RollTypeIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPBSkillTooltipWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		if (SkillNameText)
		{
			SkillNameText->SetText(DummySkillName);
		}

		if (SkillTypeText)
		{
			SkillTypeText->SetText(DummySkillType);
		}
		
		if (DamageDescText)
		{
			DamageDescText->SetText(DummyDamageDesc);
		}

		if (DiceRollDescText)
		{
			DiceRollDescText->SetText(DummyDiceRollDesc);
			DiceRollDescText->SetColorAndOpacity(DummyDiceRollColor);
		}

		if (DiceRollIcon)
		{
			if (DummyDiceRollIcon)
			{
				DiceRollIcon->SetBrushFromTexture(DummyDiceRollIcon);
				DiceRollIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				DiceRollIcon->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		if (SkillDescriptionText)
		{
			SkillDescriptionText->SetText(DummyDescription);
		}

		if (ActionRangeText)
		{
			ActionRangeText->SetText(DummyActionRange);
		}

		if (RollTypeText)
		{
			RollTypeText->SetText(DummyRollType);
		}
	}
}
