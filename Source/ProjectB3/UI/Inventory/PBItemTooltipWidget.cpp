// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBItemTooltipWidget.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/UI/Inventory/PBInventoryViewModel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPBItemTooltipWidget::BindViewModel(UPBInventoryViewModel* InViewModel)
{
	if (InViewModel)
	{
		InViewModel->OnTooltipDataGenerated.AddUObject(this, &UPBItemTooltipWidget::SetTooltipData);
	}
}

void UPBItemTooltipWidget::SetTooltipData(const FPBItemTooltipData& InData)
{
	// 공통 하단 장식 오버레이
	if (TooltipBottomDecor)
	{
		if (!BottomDecorTexture.IsNull())
		{
			TooltipBottomDecor->SetBrushFromSoftTexture(BottomDecorTexture);
			TooltipBottomDecor->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			TooltipBottomDecor->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// (헤더 및 통합 스탯)
	if (Box1_RarityOverlay)
	{
		Box1_RarityOverlay->SetColorAndOpacity(InData.RarityOverlayColor);
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(!InData.ItemName.IsEmpty() ? InData.ItemName : FallbackItemName);
	}

	if (RarityText)
	{
		RarityText->SetText(!InData.RarityText.IsEmpty() ? InData.RarityText : FallbackRarity);
	}

	// 무기/방어구 스탯 래퍼 박스
	if (Box_Equipment)
	{
		if (InData.ItemType == EPBItemType::Consumable)
		{
			Box_Equipment->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Box_Equipment->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
	}

	// DamageRangeText: 무기="4~9 피해", 방어구="방어도 15", 기타=Fallback or Collapsed
	if (DamageRangeText)
	{
		if (InData.ItemType == EPBItemType::Consumable)
		{
			DamageRangeText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if (!InData.DamageRangeText.IsEmpty())
		{
			DamageRangeText->SetText(InData.DamageRangeText);
			DamageRangeText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else if (!FallbackDamageRange.IsEmpty())
		{
			DamageRangeText->SetText(FallbackDamageRange);
			DamageRangeText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DamageRangeText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// DiceIcon: 무기=주사위, 등 기타
	if (DiceIcon)
	{
		if (InData.ItemType == EPBItemType::Consumable)
		{
			DiceIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 동적 데이터가 없더라도 툴팁 위젯의 FallbackDiceIcon 속성에 할당해둔 이미지가 나오도록 복구
			const TSoftObjectPtr<UTexture2D>& ActiveDiceIcon = !InData.DiceIcon.IsNull() ? InData.DiceIcon : FallbackDiceIcon;
			if (!ActiveDiceIcon.IsNull())
			{
				DiceIcon->SetBrushFromSoftTexture(ActiveDiceIcon);
				DiceIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			else
			{
				DiceIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
	}

	// DiceText: 무기="1d6", 방어구="경갑" 등
	if (DiceText)
	{
		if (InData.ItemType == EPBItemType::Consumable)
		{
			DiceText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else if (!InData.DiceText.IsEmpty())
		{
			DiceText->SetText(InData.DiceText);
			// FSlateColor 구조체 초기화 불량이나 런타임 캐시 오류로 인한 마젠타 버그를 방지하기 위해 가장 안전한 렌더러 컬러로 강제 주입
			DiceText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
			DiceText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else if (!FallbackDice.IsEmpty())
		{
			DiceText->SetText(FallbackDice);
			DiceText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
			DiceText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DiceText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// ModifierText: 무기="(+3)", 방어구=은신 경고, 소모품/기타=빈값→숨김
	if (ModifierText)
	{
		if (!InData.ModifierText.IsEmpty())
		{
			ModifierText->SetText(InData.ModifierText);
			ModifierText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ModifierText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// DamageTypeIcon: 무기에서만 사용, 나머지는 빈값→숨김
	if (DamageTypeIcon)
	{
		const TSoftObjectPtr<UTexture2D>& ActiveDamageTypeIcon = !InData.DamageTypeIcon.IsNull() ? InData.DamageTypeIcon : FallbackDamageTypeIcon;
		if (!ActiveDamageTypeIcon.IsNull())
		{
			DamageTypeIcon->SetBrushFromSoftTexture(ActiveDamageTypeIcon);
			DamageTypeIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DamageTypeIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// DamageTypeText: 무기에서만 사용, 나머지는 빈값→숨김
	if (DamageTypeText)
	{
		if (!InData.DamageTypeText.IsEmpty())
		{
			DamageTypeText->SetText(InData.DamageTypeText);
			DamageTypeText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else if (!FallbackDamageType.IsEmpty())
		{
			DamageTypeText->SetText(FallbackDamageType);
			DamageTypeText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			DamageTypeText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (ItemModelImage)
	{
		const TSoftObjectPtr<UTexture2D>& ActiveModelIcon = !InData.ItemModelIcon.IsNull() ? InData.ItemModelIcon : FallbackItemModelIcon;
		if (!ActiveModelIcon.IsNull())
		{
			ItemModelImage->SetBrushFromSoftTexture(ActiveModelIcon);
			ItemModelImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			ItemModelImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// (소모품 전용 박스)
	if (Box_Consumable)
	{
		if (InData.ItemType == EPBItemType::Consumable)
		{
			Box_Consumable->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			if (ConsumableEffectText)
			{
				ConsumableEffectText->SetText(InData.ConsumableEffectText);
				ConsumableEffectText->SetVisibility(!InData.ConsumableEffectText.IsEmpty() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
			}
			if (ConsumableEffectIcon)
			{
				if (!InData.ConsumableEffectIcon.IsNull())
				{
					ConsumableEffectIcon->SetBrushFromSoftTexture(InData.ConsumableEffectIcon);
					ConsumableEffectIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
				else
				{
					ConsumableEffectIcon->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			if (DurationText)
			{
				DurationText->SetText(InData.DurationText);
				DurationText->SetVisibility(!InData.DurationText.IsEmpty() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
			}
		}
		else
		{
			Box_Consumable->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// (어빌리티 설명)
	if (Box_Ability)
	{
		// 소모품은 전용 박스(Box_Consumable)를 사용하므로 기존 Ability 박스는 숨김
		if (InData.ItemType == EPBItemType::Consumable)
		{
			Box_Ability->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			// 데이터가 없어도 기본 Fallback 텍스트로 박스를 유지하려면, Fallback 텍스트 자체가 비어있지 않은지 판단함.
			bool bDisplayAbility = InData.bHasAbility || !FallbackAbilityDesc.IsEmpty();
			if (bDisplayAbility)
			{
				Box_Ability->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				if (AbilityDescText)
				{
					AbilityDescText->SetText(InData.bHasAbility ? InData.AbilityDescription : FallbackAbilityDesc);
				}
			}
			else
			{
				Box_Ability->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	// (로어 및 무기 정보)
	if (Box_Lore)
	{
		bool bDisplayLoreBox = InData.bHasLore || (!FallbackLoreDesc.IsEmpty() || !FallbackItemCategory.IsEmpty());
		if (bDisplayLoreBox)
		{
			Box_Lore->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			
			if (LoreIcon)
			{
				const TSoftObjectPtr<UTexture2D>& ActiveLoreIcon = !InData.LoreIcon.IsNull() ? InData.LoreIcon : FallbackLoreIcon;
				if (!ActiveLoreIcon.IsNull())
				{
					LoreIcon->SetBrushFromSoftTexture(ActiveLoreIcon);
					LoreIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					
					if (LoreIcon_Consumable)
					{
						LoreIcon_Consumable->SetBrushFromSoftTexture(ActiveLoreIcon);
						LoreIcon_Consumable->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
				}
				else
				{
					LoreIcon->SetVisibility(ESlateVisibility::Collapsed);
					if (LoreIcon_Consumable) LoreIcon_Consumable->SetVisibility(ESlateVisibility::Collapsed);
				}
			}

			if (LoreDescText)
			{
				LoreDescText->SetText(!InData.LoreDescription.IsEmpty() ? InData.LoreDescription : FallbackLoreDesc);
			}

			if (ItemCategoryText)
			{
				ItemCategoryText->SetText(!InData.ItemCategoryText.IsEmpty() ? InData.ItemCategoryText : FallbackItemCategory);
			}
		}
		else
		{
			Box_Lore->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPBItemTooltipWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 디자인 (위젯 에디터 내부)에서만 동작: 텍스트 덮어쓰기 없이 지정한 PreviewRarity의 오버레이 색상만 갱신
	if (IsDesignTime())
	{
		if (Box1_RarityOverlay)
		{
			if (const FLinearColor* MappedColor = RarityOverlayColorMap.Find(PreviewRarity))
			{
				Box1_RarityOverlay->SetColorAndOpacity(*MappedColor);
			}
			else
			{
				Box1_RarityOverlay->SetColorAndOpacity(FLinearColor::Transparent);
			}
		}
	}
}
