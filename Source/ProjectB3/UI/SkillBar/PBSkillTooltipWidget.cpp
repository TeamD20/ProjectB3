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
		if (!InSlotData.SkillType.IsEmpty())
		{
			SkillTypeText->SetText(InSlotData.SkillType);
			SkillTypeText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			SkillTypeText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 스킬 설명
	if (SkillDescriptionText)
	{
		if (!InSlotData.Description.IsEmpty())
		{
			SkillDescriptionText->SetText(InSlotData.Description);
			SkillDescriptionText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			SkillDescriptionText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 피해량 설명
	if (DamageDescText)
	{
		if (!InSlotData.DamageDesc.IsEmpty())
		{
			DamageDescText->SetText(InSlotData.DamageDesc);
			DamageDescText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DamageDescText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 주사위 설명 & 아이콘
	if (DiceRollDescText)
	{
		if (!InSlotData.DiceRollDesc.IsEmpty())
		{
			DiceRollDescText->SetText(InSlotData.DiceRollDesc);
			// 디자이너 지정 색상을 보호하기 위해 동적 C++ 덮어씌우기 제거
			// DiceRollDescText->SetColorAndOpacity(InSlotData.DiceRollColor); 
			DiceRollDescText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DiceRollDescText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (DiceRollIcon)
	{
		// 텍스트가 존재할 경우 다이스 아이콘은 동적 데이터 할당 없이 BP에 할당된 이미지가 뜨도록 고정
		if (!InSlotData.DiceRollDesc.IsEmpty())
		{
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
		if (!InSlotData.ActionRange.IsEmpty())
		{
			ActionRangeText->SetText(InSlotData.ActionRange);
			ActionRangeText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (ActionRangeIcon) ActionRangeIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ActionRangeText->SetVisibility(ESlateVisibility::Collapsed);
			if (ActionRangeIcon) ActionRangeIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 7. 굴림 유형 (Roll Type)
	if (RollTypeText)
	{
		if (!InSlotData.RollType.IsEmpty())
		{
			RollTypeText->SetText(InSlotData.RollType);
			RollTypeText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			RollTypeText->SetVisibility(ESlateVisibility::Collapsed);
		}
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
