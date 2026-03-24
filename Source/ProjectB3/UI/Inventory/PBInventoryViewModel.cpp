// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInventoryViewModel.h"

#include "ProjectB3/Characters/PBCharacterPreviewComponent.h"
#include "ProjectB3/Characters/PBCharacterBase.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"
#include "ProjectB3/ItemSystem/Data/PBEquipmentDataAsset.h"
#include "ProjectB3/ItemSystem/Data/PBConsumableDataAsset.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/Game/PBGameInstance.h"
#include "ProjectB3/AbilitySystem/Data/PBAbilitySystemRegistry.h"
#include "ProjectB3/AbilitySystem/Abilities/PBGameplayAbility.h"

void UPBInventoryViewModel::InitializeForActor(AActor* InTargetActor, ULocalPlayer* InLocalPlayer)
{
	Super::InitializeForActor(InTargetActor, InLocalPlayer);

	CachedInventory = nullptr;
	CachedEquipment = nullptr;
	CachedPreviewComponent = nullptr;
	ActiveWeaponSet = 1;

	if (!IsValid(InTargetActor))
	{
		RebuildAllSlots();
		return;
	}

	// 대상 Actor에서 인벤토리/장비/프리뷰 컴포넌트를 획득
	CachedInventory = InTargetActor->FindComponentByClass<UPBInventoryComponent>();
	CachedEquipment = InTargetActor->FindComponentByClass<UPBEquipmentComponent>();
	CachedPreviewComponent = InTargetActor->FindComponentByClass<UPBCharacterPreviewComponent>();

	if (IsValid(CachedInventory))
	{
		// 인벤토리는 개별 슬롯 변경 + 전체 리프레시를 모두 구독
		CachedInventory->OnInventoryItemChanged.AddDynamic(this, &ThisClass::HandleInventorySlotChanged);
		CachedInventory->OnInventoryFullRefresh.AddDynamic(this, &ThisClass::HandleInventoryFullRefreshInternal);
	}

	if (IsValid(CachedEquipment))
	{
		// 장비 변경과 무기 세트 전환 이벤트를 구독
		CachedEquipment->OnEquipmentSlotChanged.AddDynamic(this, &ThisClass::HandleEquipmentSlotChanged);
		CachedEquipment->OnWeaponSetSwitched.AddDynamic(this, &ThisClass::HandleWeaponSetSwitched);
		ActiveWeaponSet = CachedEquipment->GetActiveWeaponSet();
	}

	UpdateCharacterName(InTargetActor);
	RebuildAllSlots();
	// 초기 표시 시 무기 세트 하이라이트 동기화를 위해 현재 값을 즉시 브로드캐스트
	OnWeaponSetChanged.Broadcast(ActiveWeaponSet);
}

void UPBInventoryViewModel::Deinitialize()
{
	if (IsValid(CachedInventory))
	{
		CachedInventory->OnInventoryItemChanged.RemoveDynamic(this, &ThisClass::HandleInventorySlotChanged);
		CachedInventory->OnInventoryFullRefresh.RemoveDynamic(this, &ThisClass::HandleInventoryFullRefreshInternal);
	}

	if (IsValid(CachedEquipment))
	{
		CachedEquipment->OnEquipmentSlotChanged.RemoveDynamic(this, &ThisClass::HandleEquipmentSlotChanged);
		CachedEquipment->OnWeaponSetSwitched.RemoveDynamic(this, &ThisClass::HandleWeaponSetSwitched);
	}

	CachedInventory = nullptr;
	CachedEquipment = nullptr;
	CachedPreviewComponent = nullptr;
	InventorySlots.Reset();
	EquipmentSlots.Reset();
	CharacterName = FText::GetEmpty();
	ActiveWeaponSet = 1;

	Super::Deinitialize();
}

UTextureRenderTarget2D* UPBInventoryViewModel::GetCharacterRenderTarget() const
{
	return IsValid(CachedPreviewComponent) ? CachedPreviewComponent->GetRenderTarget() : nullptr;
}

void UPBInventoryViewModel::SetPreviewCaptureActive(bool bActive)
{
	if (IsValid(CachedPreviewComponent))
	{
		CachedPreviewComponent->SetCaptureActive(bActive);
	}
}

bool UPBInventoryViewModel::GetInventorySlotData(int32 SlotIndex, FPBInventorySlotData& OutData) const
{
	if (!InventorySlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	OutData = InventorySlots[SlotIndex];
	return true;
}

bool UPBInventoryViewModel::GetEquipmentSlotData(EPBEquipSlot Slot, FPBInventorySlotData& OutData) const
{
	if (const FPBInventorySlotData* Found = EquipmentSlots.Find(Slot))
	{
		OutData = *Found;
		return true;
	}

	return false;
}

void UPBInventoryViewModel::RequestTooltipData(const FGuid& InstanceID)
{
	if (!InstanceID.IsValid()) return;

	FPBItemInstance TargetInstance;
	
	if (IsValid(CachedInventory))
	{
		TargetInstance = CachedInventory->FindItemByID(InstanceID);
	}

	if (!TargetInstance.IsValid() && IsValid(CachedEquipment))
	{
		for (const auto& Pair : CachedEquipment->GetEquippedItems())
		{
			if (Pair.Value.InstanceID == InstanceID)
			{
				TargetInstance = Pair.Value;
				break;
			}
		}
	}

	if (!TargetInstance.IsValid() || !TargetInstance.ItemDataAsset) return;

	UPBItemDataAsset* ItemData = TargetInstance.ItemDataAsset;

	FPBItemTooltipData TooltipData;
	TooltipData.ItemName = ItemData->ItemName;
	TooltipData.ItemModelIcon = ItemData->ItemIcon;
	TooltipData.ItemType = ItemData->ItemType;
	
	switch (ItemData->Rarity)
	{
		case EPBItemRarity::Common:    TooltipData.RarityText = FText::FromString(TEXT("일반")); break;
		case EPBItemRarity::Uncommon:  TooltipData.RarityText = FText::FromString(TEXT("고급")); break;
		case EPBItemRarity::Rare:      TooltipData.RarityText = FText::FromString(TEXT("희귀")); break;
		case EPBItemRarity::Legendary: TooltipData.RarityText = FText::FromString(TEXT("전설")); break;
		default: TooltipData.RarityText = FText::FromString(TEXT("Null")); break;
	}

	FLinearColor OverlayColor = GetRarityColor(ItemData->Rarity);
	if (ItemData->Rarity == EPBItemRarity::Common) OverlayColor = FLinearColor::Transparent;
	else OverlayColor.A = 0.45f;
	TooltipData.RarityOverlayColor = OverlayColor;

	TooltipData.AbilityDescription = FText::GetEmpty();
	TooltipData.bHasAbility = false;
	TooltipData.LoreDescription = ItemData->BackGroundDescription;

	switch (ItemData->ItemType)
	{
		case EPBItemType::Weapon:     TooltipData.ItemCategoryText = FText::FromString(TEXT("무기")); break;
		case EPBItemType::Armor:      TooltipData.ItemCategoryText = FText::FromString(TEXT("방어구")); break;
		case EPBItemType::Trinket:    TooltipData.ItemCategoryText = FText::FromString(TEXT("장신구")); break;
		case EPBItemType::Consumable: TooltipData.ItemCategoryText = FText::FromString(TEXT("소모품")); break;
		case EPBItemType::Misc:       TooltipData.ItemCategoryText = FText::FromString(TEXT("기타")); break;
		default:                      TooltipData.ItemCategoryText = FText::GetEmpty(); break;
	}

	TooltipData.bHasLore = !TooltipData.LoreDescription.IsEmpty() || !TooltipData.ItemCategoryText.IsEmpty();

	if (UPBEquipmentDataAsset* EquipData = Cast<UPBEquipmentDataAsset>(ItemData))
	{
		FString CombinedAbilities;
		for (const FPBAbilityGrantEntry& Entry : EquipData->GrantedAbilities)
		{
			if (IsValid(Entry.AbilityClass))
			{
				if (UPBGameplayAbility* AbilityCDO = Entry.AbilityClass->GetDefaultObject<UPBGameplayAbility>())
				{
					if (!CombinedAbilities.IsEmpty()) CombinedAbilities += TEXT("\n\n");
					CombinedAbilities += FString::Printf(TEXT("[%s]\n%s"), 
						*AbilityCDO->GetAbilityDisplayName().ToString(), 
						*AbilityCDO->GetAbilityDescription().ToString());
				}
			}
		}
		TooltipData.AbilityDescription = FText::FromString(CombinedAbilities);
		TooltipData.bHasAbility = !CombinedAbilities.IsEmpty();

		if (EquipData->EquipmentType == EPBEquipmentType::Weapon)
		{
			const FPBDiceSpec Dice = EquipData->GetAbilityDamageSpec();
			TooltipData.DiceText = FText::FromString(FString::Printf(TEXT("%dd%d"), Dice.DiceCount, Dice.DiceFaces));
			TooltipData.DiceColor = Dice.DiceColor;
			TooltipData.DiceIcon = Dice.DiceIcon;

			int32 Mod = EquipData->GetWeaponBonusModifier();
			if (Mod > 0) TooltipData.ModifierText = FText::FromString(FString::Printf(TEXT("(+%d)"), Mod));
			else if (Mod < 0) TooltipData.ModifierText = FText::FromString(FString::Printf(TEXT("(%d)"), Mod));
			else TooltipData.ModifierText = FText::GetEmpty();

			TooltipData.DamageRangeText = FText::FromString(FString::Printf(TEXT("%d~%d 피해"), 
				FMath::Max(1, Dice.DiceCount * 1 + Mod), 
				FMath::Max(1, Dice.DiceCount * Dice.DiceFaces + Mod)));

			TooltipData.DamageTypeTag = EquipData->GetDamageTypeTag();
			
			if (const UPBAbilitySystemRegistry* Registry = UPBGameInstance::GetAbilitySystemRegistry(this))
			{
				TooltipData.DamageTypeText = Registry->GetTagDisplayName(TooltipData.DamageTypeTag);
				TooltipData.DamageTypeIcon = Registry->GetTagIcon(TooltipData.DamageTypeTag);
			}
		}
		else if (EquipData->EquipmentType != EPBEquipmentType::Weapon)
		{
			TooltipData.DamageRangeText = FText::FromString(FString::Printf(TEXT("방어도 %d"), EquipData->GetArmorClass()));
		}
	}
	else if (UPBConsumableDataAsset* ConsumableData = Cast<UPBConsumableDataAsset>(ItemData))
	{
		if (IsValid(ConsumableData->ConsumableAbilityClass))
		{
			if (UPBGameplayAbility* AbilityCDO = ConsumableData->ConsumableAbilityClass->GetDefaultObject<UPBGameplayAbility>())
			{
				TooltipData.ConsumableEffectText = AbilityCDO->GetAbilityDescription();
				TooltipData.ConsumableEffectIcon = AbilityCDO->GetAbilityIcon();
				
				int32 Duration = AbilityCDO->GetEffectDuration();
				TooltipData.DurationText = Duration > 0 ? FText::FromString(FString::Printf(TEXT("%d턴 지속"), Duration)) : FText::FromString(TEXT("즉시 적용"));
			}
		}
	}

	OnTooltipDataGenerated.Broadcast(TooltipData);
}

FPBInventorySlotData UPBInventoryViewModel::BuildSlotData(const FPBItemInstance& ItemInstance) const
{
	// Component의 런타임 인스턴스를 UI 표시용 경량 스냅샷으로 변환
	FPBInventorySlotData Result;

	if (!ItemInstance.IsValid() || !IsValid(ItemInstance.ItemDataAsset))
	{
		return Result;
	}

	Result.bIsEmpty = false;
	Result.ItemIcon = ItemInstance.ItemDataAsset->ItemIcon;
	Result.ItemName = ItemInstance.ItemDataAsset->ItemName;
	Result.StackCount = ItemInstance.Count;
	Result.RarityColor = GetRarityColor(ItemInstance.ItemDataAsset->Rarity);
	Result.InstanceID = ItemInstance.InstanceID;
	Result.ItemType = ItemInstance.ItemDataAsset->ItemType;
	return Result;
}

FLinearColor UPBInventoryViewModel::GetRarityColor(EPBItemRarity Rarity) const
{
	switch (Rarity)
	{
	case EPBItemRarity::Common:
		return FLinearColor(0.6f, 0.6f, 0.6f, 1.0f);
	case EPBItemRarity::Uncommon:
		return FLinearColor(0.2f, 0.8f, 0.2f, 1.0f);
	case EPBItemRarity::Rare:
		return FLinearColor(0.3f, 0.5f, 1.0f, 1.0f);
	case EPBItemRarity::Legendary:
		return FLinearColor(1.0f, 0.7f, 0.1f, 1.0f);
	default:
		return FLinearColor::White;
	}
}

void UPBInventoryViewModel::RebuildAllSlots()
{
	RebuildInventorySlots();
	RebuildEquipmentSlots();
}

void UPBInventoryViewModel::RebuildInventorySlots()
{
	const int32 MaxSlots = IsValid(CachedInventory) ? CachedInventory->GetMaxSlots() : 0;
	InventorySlots.SetNum(MaxSlots);

	if (!IsValid(CachedInventory))
	{
		return;
	}

	// UsedSlots 범위는 실제 아이템, 나머지는 빈 슬롯 데이터로 유지
	const int32 UsedSlots = CachedInventory->GetUsedSlotCount();
	for (int32 SlotIndex = 0; SlotIndex < MaxSlots; ++SlotIndex)
	{
		if (SlotIndex < UsedSlots)
		{
			InventorySlots[SlotIndex] = BuildSlotData(CachedInventory->GetItemAtSlot(SlotIndex));
		}
		else
		{
			InventorySlots[SlotIndex] = FPBInventorySlotData();
		}
	}
}

void UPBInventoryViewModel::RebuildEquipmentSlots()
{
	EquipmentSlots.Reset();

	// EPBEquipSlot 전체 키를 항상 채워 위젯 쪽 키 조회를 단순화
	for (int32 SlotValue = 0; SlotValue < static_cast<int32>(EPBEquipSlot::MAX); ++SlotValue)
	{
		const EPBEquipSlot Slot = static_cast<EPBEquipSlot>(SlotValue);
		FPBInventorySlotData SlotData;

		if (IsValid(CachedEquipment))
		{
			SlotData = BuildSlotData(CachedEquipment->GetEquippedItem(Slot));
		}

		EquipmentSlots.Add(Slot, SlotData);
	}
}

void UPBInventoryViewModel::UpdateCharacterName(AActor* InActor)
{
	FText NewName = FText::GetEmpty();
	if (IsValid(InActor))
	{
		// CombatIdentity의 DisplayName을 우선 사용, 없으면 액터 오브젝트명으로 폴백
		if (const APBCharacterBase* Character = Cast<APBCharacterBase>(InActor))
		{
			NewName = Character->GetCombatDisplayName();
		}
		if (NewName.IsEmpty())
		{
			NewName = FText::FromString(InActor->GetName());
		}
	}

	if (!CharacterName.EqualTo(NewName))
	{
		CharacterName = NewName;
		OnCharacterNameChanged.Broadcast(CharacterName);
	}
}

void UPBInventoryViewModel::HandleInventorySlotChanged(int32 SlotIndex)
{
	if (!InventorySlots.IsValidIndex(SlotIndex) || !IsValid(CachedInventory))
	{
		return;
	}

	InventorySlots[SlotIndex] = BuildSlotData(CachedInventory->GetItemAtSlot(SlotIndex));
	OnInventorySlotUpdated.Broadcast(SlotIndex);
}

void UPBInventoryViewModel::HandleInventoryFullRefreshInternal()
{
	// RemoveAt 등으로 인덱스가 당겨진 경우 전체 캐시를 다시 구성
	RebuildInventorySlots();
	OnInventoryFullRefresh.Broadcast();
}

void UPBInventoryViewModel::HandleEquipmentSlotChanged(EPBEquipSlot Slot)
{
	if (!IsValid(CachedEquipment))
	{
		return;
	}

	EquipmentSlots.FindOrAdd(Slot) = BuildSlotData(CachedEquipment->GetEquippedItem(Slot));
	OnEquipmentSlotUpdated.Broadcast(Slot);
}

void UPBInventoryViewModel::HandleWeaponSetSwitched(int32 NewActiveSet)
{
	if (ActiveWeaponSet == NewActiveSet)
	{
		return;
	}

	ActiveWeaponSet = NewActiveSet;
	OnWeaponSetChanged.Broadcast(ActiveWeaponSet);
}
