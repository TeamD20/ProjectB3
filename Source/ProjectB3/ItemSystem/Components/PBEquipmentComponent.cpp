// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBEquipmentComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Data/PBEquipmentDataAsset.h"
#include "ProjectB3/PBGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogPBEquipment, Log, All);

UPBEquipmentComponent::UPBEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPBEquipmentComponent::EquipItem(const FGuid& InstanceID, EPBEquipSlot Slot, UPBInventoryComponent* Inventory)
{
	if (!IsValid(Inventory))
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("EquipItem: 인벤토리 컴포넌트가 유효하지 않음"));
		return false;
	}

	// 인벤토리에서 아이템 조회
	const FPBItemInstance SourceItem = Inventory->FindItemByID(InstanceID);
	if (!SourceItem.IsValid())
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("EquipItem: InstanceID에 해당하는 아이템을 찾을 수 없음"));
		return false;
	}

	// 장비 DataAsset 검증
	const UPBEquipmentDataAsset* EquipData = Cast<UPBEquipmentDataAsset>(SourceItem.ItemDataAsset);
	if (!IsValid(EquipData))
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("EquipItem: 장비 DataAsset이 아닌 아이템"));
		return false;
	}

	// 슬롯 호환성 검증
	if (!EquipData->CanEquipToSlot(Slot))
	{
		UE_LOG(LogPBEquipment, Log, TEXT("EquipItem: %s 슬롯에 장착 불가"), *UEnum::GetValueAsString(Slot));
		return false;
	}

	// 양손무기는 Main 슬롯에만 장착 가능
	if (EquipData->WeaponHandType == EPBWeaponHandType::TwoHanded)
	{
		if (Slot == EPBEquipSlot::WeaponSet1_Off || Slot == EPBEquipSlot::WeaponSet2_Off)
		{
			UE_LOG(LogPBEquipment, Log, TEXT("EquipItem: 양손무기는 Off 슬롯에 장착 불가"));
			return false;
		}
	}

	// Main 슬롯에 양손무기가 있으면 Off 슬롯 장착 불가
	if (Slot == EPBEquipSlot::WeaponSet1_Off || Slot == EPBEquipSlot::WeaponSet2_Off)
	{
		const EPBEquipSlot PairedMain = (Slot == EPBEquipSlot::WeaponSet1_Off)
			? EPBEquipSlot::WeaponSet1_Main : EPBEquipSlot::WeaponSet2_Main;

		if (const FPBItemInstance* MainItem = EquippedItems.Find(PairedMain))
		{
			if (const UPBEquipmentDataAsset* MainEquipData = Cast<UPBEquipmentDataAsset>(MainItem->ItemDataAsset))
			{
				if (MainEquipData->WeaponHandType == EPBWeaponHandType::TwoHanded)
				{
					UE_LOG(LogPBEquipment, Log, TEXT("EquipItem: 양손무기가 Main에 장착되어 Off 슬롯 사용 불가"));
					return false;
				}
			}
		}
	}

	// 기존 장비가 있으면 먼저 해제
	if (!IsSlotEmpty(Slot))
	{
		if (!UnequipItem(Slot, Inventory))
		{
			UE_LOG(LogPBEquipment, Warning, TEXT("EquipItem: 기존 장비 해제 실패"));
			return false;
		}
	}

	// 양손무기 장착 시 대응하는 Off 슬롯도 자동 해제
	if (EquipData->WeaponHandType == EPBWeaponHandType::TwoHanded)
	{
		const EPBEquipSlot PairedOff = (Slot == EPBEquipSlot::WeaponSet1_Main)
			? EPBEquipSlot::WeaponSet1_Off : EPBEquipSlot::WeaponSet2_Off;

		if (!IsSlotEmpty(PairedOff))
		{
			if (!UnequipItem(PairedOff, Inventory))
			{
				UE_LOG(LogPBEquipment, Warning, TEXT("EquipItem: 양손무기 장착 — Off 슬롯 해제 실패"));
				return false;
			}
		}
	}

	// 원본 인스턴스 정보 보존 후 인벤토리에서 제거
	FPBItemInstance EquipInstance;
	EquipInstance.InstanceID = SourceItem.InstanceID;
	EquipInstance.ItemDataAsset = SourceItem.ItemDataAsset;
	EquipInstance.Count = 1;
	Inventory->RemoveItem(InstanceID, 1);

	// 장비 슬롯에 삽입
	EquippedItems.Add(Slot, EquipInstance);

	// GAS 부여 (활성 무기 세트에 속하는 슬롯이거나 비무기 슬롯일 때만)
	if (!IsWeaponSlot(Slot) || GetWeaponSetNumber(Slot) == ActiveWeaponSet)
	{
		GrantEquipmentAbilities(Slot, EquipData);
	}

	OnEquipmentSlotChanged.Broadcast(Slot);

	UE_LOG(LogPBEquipment, Log, TEXT("EquipItem: %s → %s 슬롯 장착 완료"),
		*EquipData->GetName(), *UEnum::GetValueAsString(Slot));

	return true;
}

bool UPBEquipmentComponent::UnequipItem(EPBEquipSlot Slot, UPBInventoryComponent* Inventory)
{
	if (!IsValid(Inventory))
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("UnequipItem: 인벤토리 컴포넌트가 유효하지 않음"));
		return false;
	}

	const FPBItemInstance* EquippedItem = EquippedItems.Find(Slot);
	if (!EquippedItem)
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("UnequipItem: %s 슬롯이 비어있음"), *UEnum::GetValueAsString(Slot));
		return false;
	}

	// 인벤토리에 여유 공간 확인
	if (!Inventory->HasFreeSlot())
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("UnequipItem: 인벤토리 공간 부족"));
		return false;
	}

	// GAS에서 이펙트/어빌리티 제거
	RevokeEquipmentAbilities(Slot);

	// 인벤토리에 복귀 (InstanceID 보존)
	const FPBItemInstance ItemToReturn = *EquippedItem;
	EquippedItems.Remove(Slot);
	Inventory->AddItemInstance(ItemToReturn);

	OnEquipmentSlotChanged.Broadcast(Slot);

	UE_LOG(LogPBEquipment, Log, TEXT("UnequipItem: %s 슬롯 해제 → 인벤토리 복귀"),
		*UEnum::GetValueAsString(Slot));

	return true;
}

void UPBEquipmentComponent::SwitchWeaponSet()
{
	const int32 PreviousSet = ActiveWeaponSet;
	ActiveWeaponSet = (ActiveWeaponSet == 1) ? 2 : 1;

	// 이전 세트의 GAS 제거
	const EPBEquipSlot PrevMain = (PreviousSet == 1) ? EPBEquipSlot::WeaponSet1_Main : EPBEquipSlot::WeaponSet2_Main;
	const EPBEquipSlot PrevOff = (PreviousSet == 1) ? EPBEquipSlot::WeaponSet1_Off : EPBEquipSlot::WeaponSet2_Off;
	RevokeEquipmentAbilities(PrevMain);
	RevokeEquipmentAbilities(PrevOff);

	// 새 세트의 GAS 부여
	const EPBEquipSlot NewMain = (ActiveWeaponSet == 1) ? EPBEquipSlot::WeaponSet1_Main : EPBEquipSlot::WeaponSet2_Main;
	const EPBEquipSlot NewOff = (ActiveWeaponSet == 1) ? EPBEquipSlot::WeaponSet1_Off : EPBEquipSlot::WeaponSet2_Off;

	if (const FPBItemInstance* MainItem = EquippedItems.Find(NewMain))
	{
		if (const UPBEquipmentDataAsset* EquipData = Cast<UPBEquipmentDataAsset>(MainItem->ItemDataAsset))
		{
			GrantEquipmentAbilities(NewMain, EquipData);
		}
	}

	if (const FPBItemInstance* OffItem = EquippedItems.Find(NewOff))
	{
		if (const UPBEquipmentDataAsset* EquipData = Cast<UPBEquipmentDataAsset>(OffItem->ItemDataAsset))
		{
			GrantEquipmentAbilities(NewOff, EquipData);
		}
	}

	OnWeaponSetSwitched.Broadcast(ActiveWeaponSet);

	UE_LOG(LogPBEquipment, Log, TEXT("SwitchWeaponSet: 세트 %d → %d 전환"), PreviousSet, ActiveWeaponSet);
}

FPBItemInstance UPBEquipmentComponent::GetEquippedItem(EPBEquipSlot Slot) const
{
	if (const FPBItemInstance* Found = EquippedItems.Find(Slot))
	{
		return *Found;
	}

	return FPBItemInstance();
}

bool UPBEquipmentComponent::IsSlotEmpty(EPBEquipSlot Slot) const
{
	return !EquippedItems.Contains(Slot);
}

EPBEquipSlot UPBEquipmentComponent::ResolveAutoEquipSlot(const UPBEquipmentDataAsset* EquipData) const
{
	if (!IsValid(EquipData))
	{
		return EPBEquipSlot::MAX;
	}

	const TArray<EPBEquipSlot> AllowedSlots = UPBEquipmentDataAsset::GetAllowedSlotsForType(
		EquipData->EquipmentType, EquipData->WeaponHandType);

	if (AllowedSlots.IsEmpty())
	{
		return EPBEquipSlot::MAX;
	}

	// 방어구/장신구 등 단일 슬롯 장비 — 빈 슬롯이면 장착, 아니면 교체
	if (AllowedSlots.Num() == 1)
	{
		return AllowedSlots[0];
	}

	// 반지: Ring1 → Ring2 순으로 빈 슬롯 탐색, 둘 다 차있으면 Ring1 교체
	if (EquipData->EquipmentType == EPBEquipmentType::Ring)
	{
		if (IsSlotEmpty(EPBEquipSlot::Ring1)) { return EPBEquipSlot::Ring1; }
		if (IsSlotEmpty(EPBEquipSlot::Ring2)) { return EPBEquipSlot::Ring2; }
		return EPBEquipSlot::Ring1;
	}

	// 무기/방패: 활성 세트의 Main → Off 순으로 탐색
	const EPBEquipSlot MainSlot = (ActiveWeaponSet == 1)
		? EPBEquipSlot::WeaponSet1_Main : EPBEquipSlot::WeaponSet2_Main;
	const EPBEquipSlot OffSlot = (ActiveWeaponSet == 1)
		? EPBEquipSlot::WeaponSet1_Off : EPBEquipSlot::WeaponSet2_Off;

	// 양손무기/방패는 해당 슬롯만 사용
	if (EquipData->EquipmentType == EPBEquipmentType::Shield)
	{
		return IsSlotEmpty(OffSlot) ? OffSlot : OffSlot; // Off 슬롯 교체
	}

	if (EquipData->WeaponHandType == EPBWeaponHandType::TwoHanded)
	{
		return EquipData->CanEquipToSlot(MainSlot) ? MainSlot : EPBEquipSlot::MAX;
	}

	// 한손/다용도: Main 비었으면 Main, 아니면 Off (Main에 양손무기 있으면 Main 교체)
	if (EquipData->CanEquipToSlot(MainSlot) && IsSlotEmpty(MainSlot))
	{
		return MainSlot;
	}

	if (EquipData->CanEquipToSlot(OffSlot) && IsSlotEmpty(OffSlot))
	{
		// Main에 양손무기가 있으면 Off 사용 불가 → Main 교체
		if (const FPBItemInstance* MainItem = EquippedItems.Find(MainSlot))
		{
			if (const UPBEquipmentDataAsset* MainData = Cast<UPBEquipmentDataAsset>(MainItem->ItemDataAsset))
			{
				if (MainData->WeaponHandType == EPBWeaponHandType::TwoHanded)
				{
					return MainSlot;
				}
			}
		}
		return OffSlot;
	}

	// 둘 다 차있으면 Main 교체
	if (EquipData->CanEquipToSlot(MainSlot))
	{
		return MainSlot;
	}

	return EPBEquipSlot::MAX;
}

bool UPBEquipmentComponent::AutoEquipItem(const FGuid& InstanceID, UPBInventoryComponent* Inventory)
{
	if (!IsValid(Inventory))
	{
		return false;
	}

	const FPBItemInstance SourceItem = Inventory->FindItemByID(InstanceID);
	if (!SourceItem.IsValid())
	{
		return false;
	}

	const UPBEquipmentDataAsset* EquipData = Cast<UPBEquipmentDataAsset>(SourceItem.ItemDataAsset);
	if (!IsValid(EquipData))
	{
		return false;
	}

	const EPBEquipSlot ResolvedSlot = ResolveAutoEquipSlot(EquipData);
	if (ResolvedSlot == EPBEquipSlot::MAX)
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("AutoEquipItem: 적절한 슬롯을 찾을 수 없음"));
		return false;
	}

	return EquipItem(InstanceID, ResolvedSlot, Inventory);
}

bool UPBEquipmentComponent::EquipItemFromExternalInventory(
	const FGuid& InstanceID,
	EPBEquipSlot Slot,
	UPBInventoryComponent* SourceInventory,
	UPBInventoryComponent* TargetInventory)
{
	if (!IsValid(SourceInventory) || !IsValid(TargetInventory) || !InstanceID.IsValid())
	{
		return false;
	}

	if (SourceInventory == TargetInventory)
	{
		return EquipItem(InstanceID, Slot, TargetInventory);
	}

	if (!UPBInventoryComponent::TransferItem(InstanceID, SourceInventory, TargetInventory))
	{
		return false;
	}

	if (EquipItem(InstanceID, Slot, TargetInventory))
	{
		return true;
	}

	// 장착 실패 시 원본 인벤토리로 복귀시켜 이동/장착을 원자적으로 보장한다.
	if (!UPBInventoryComponent::TransferItem(InstanceID, TargetInventory, SourceInventory))
	{
		UE_LOG(LogPBEquipment, Error, TEXT("EquipItemFromExternalInventory: 롤백 실패 (InstanceID=%s)"), *InstanceID.ToString());
	}

	return false;
}

void UPBEquipmentComponent::GrantEquipmentAbilities(EPBEquipSlot Slot, const UPBEquipmentDataAsset* EquipData)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!IsValid(ASC) || !IsValid(EquipData))
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("GrantEquipmentAbilities: ASC 또는 EquipData가 유효하지 않음"));
		return;
	}

	// 무기 슬롯이면 부착 위치 태그 조회
	const FGameplayTag AttachTag = EquipData->AttachSlotOverride.IsValid() ? EquipData->AttachSlotOverride : GetAttachSlotTag(Slot);

	FPBAbilityGrantedHandles Handles;

	// 장비 어빌리티 부여
	for (const FPBAbilityGrantEntry& Entry : EquipData->GrantedAbilities)
	{
		if (!Entry.IsValidData())
		{
			UE_LOG(LogPBEquipment, Warning, TEXT("[%s] 유효하지 않은 어빌리티 엔트리 건너뜀"), *EquipData->GetName());
			continue;
		}

		FGameplayAbilitySpec Spec(Entry.AbilityClass, Entry.AbilityLevel, INDEX_NONE, GetOwner());
		Spec.GetDynamicSpecSourceTags().AppendTags(Entry.DynamicTags);

		// 무기 슬롯이면 장비 부착 위치 태그 주입 (어빌리티 발동 시 자동 장착에 사용)
		if (AttachTag.IsValid())
		{
			Spec.GetDynamicSpecSourceTags().AddTag(AttachTag);
		}

		Handles.AbilityHandles.Add(ASC->GiveAbility(Spec));
	}

	// 장비 이펙트 부여
	for (const FPBEffectGrantEntry& Entry : EquipData->GrantedEffects)
	{
		if (!Entry.IsValidData())
		{
			UE_LOG(LogPBEquipment, Warning, TEXT("[%s] 유효하지 않은 GE 엔트리 건너뜀"), *EquipData->GetName());
			continue;
		}

		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(Entry.EffectClass, 1, ASC->MakeEffectContext());
		if (SpecHandle.IsValid())
		{
			Handles.EffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()));
		}
	}

	GrantedHandlesMap.Add(Slot, Handles);

	UE_LOG(LogPBEquipment, Verbose, TEXT("[%s] GAS 부여 완료 — 어빌리티 %d개, GE %d개"),
		*EquipData->GetName(), Handles.AbilityHandles.Num(), Handles.EffectHandles.Num());
}

void UPBEquipmentComponent::RevokeEquipmentAbilities(EPBEquipSlot Slot)
{
	FPBAbilityGrantedHandles* Handles = GrantedHandlesMap.Find(Slot);
	if (!Handles)
	{
		// 해당 슬롯에 부여된 핸들이 없으면 무시 (비활성 무기 세트 등)
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!IsValid(ASC))
	{
		UE_LOG(LogPBEquipment, Warning, TEXT("RevokeEquipmentAbilities: ASC가 유효하지 않음"));
		return;
	}

	// 부여된 어빌리티 해제
	for (const FGameplayAbilitySpecHandle& Handle : Handles->AbilityHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	// 부여된 이펙트 해제
	for (const FActiveGameplayEffectHandle& Handle : Handles->EffectHandles)
	{
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	UE_LOG(LogPBEquipment, Verbose, TEXT("RevokeEquipmentAbilities: %s 슬롯 GAS 해제 — 어빌리티 %d개, GE %d개"),
		*UEnum::GetValueAsString(Slot), Handles->AbilityHandles.Num(), Handles->EffectHandles.Num());

	GrantedHandlesMap.Remove(Slot);
}

UAbilitySystemComponent* UPBEquipmentComponent::GetOwnerASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}

	return nullptr;
}

bool UPBEquipmentComponent::IsWeaponSlot(EPBEquipSlot Slot)
{
	return Slot == EPBEquipSlot::WeaponSet1_Main
		|| Slot == EPBEquipSlot::WeaponSet1_Off
		|| Slot == EPBEquipSlot::WeaponSet2_Main
		|| Slot == EPBEquipSlot::WeaponSet2_Off;
}

int32 UPBEquipmentComponent::GetWeaponSetNumber(EPBEquipSlot Slot)
{
	if (Slot == EPBEquipSlot::WeaponSet1_Main || Slot == EPBEquipSlot::WeaponSet1_Off)
	{
		return 1;
	}
	if (Slot == EPBEquipSlot::WeaponSet2_Main || Slot == EPBEquipSlot::WeaponSet2_Off)
	{
		return 2;
	}

	return 0;
}

FGameplayTag UPBEquipmentComponent::GetAttachSlotTag(EPBEquipSlot Slot)
{
	switch (Slot)
	{
	case EPBEquipSlot::WeaponSet1_Main:
	case EPBEquipSlot::WeaponSet2_Main:
		return PBGameplayTags::Equipment_Slot_RightHand;

	case EPBEquipSlot::WeaponSet1_Off:
	case EPBEquipSlot::WeaponSet2_Off:
		return PBGameplayTags::Equipment_Slot_LeftHand;

	default:
		return FGameplayTag();
	}
}
