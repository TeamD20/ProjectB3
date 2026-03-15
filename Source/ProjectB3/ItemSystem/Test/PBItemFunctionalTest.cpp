// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBItemFunctionalTest.h"
#include "PBTestItemSystemActor.h"
#include "ProjectB3/ItemSystem/Components/PBInventoryComponent.h"
#include "ProjectB3/ItemSystem/Components/PBEquipmentComponent.h"
#include "ProjectB3/ItemSystem/Data/PBItemDataAsset.h"
#include "ProjectB3/ItemSystem/Data/PBEquipmentDataAsset.h"

APBItemFunctionalTestBase::APBItemFunctionalTestBase()
{
}

void APBItemFunctionalTestBase::PrepareTest()
{
	Super::PrepareTest();

	SpawnedActors.Empty();
	CreatedDataAssets.Empty();
}

void APBItemFunctionalTestBase::CleanUp()
{
	for (TObjectPtr<AActor>& Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();
	CreatedDataAssets.Empty();

	Super::CleanUp();
}

AActor* APBItemFunctionalTestBase::SpawnTestActor()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Actor = GetWorld()->SpawnActor<APBTestItemSystemActor>(
		APBTestItemSystemActor::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Params);

	if (IsValid(Actor))
	{
		SpawnedActors.Add(Actor);
	}

	return Actor;
}

UPBItemDataAsset* APBItemFunctionalTestBase::CreateConsumableData(const FName& ID, int32 MaxStack)
{
	UPBItemDataAsset* Data = NewObject<UPBItemDataAsset>(this);
	Data->ItemName = FText::FromName(ID);
	Data->ItemType = EPBItemType::Consumable;
	Data->MaxStackSize = MaxStack;
	CreatedDataAssets.Add(Data);
	return Data;
}

UPBEquipmentDataAsset* APBItemFunctionalTestBase::CreateOneHandedWeaponData(const FName& ID)
{
	UPBEquipmentDataAsset* Data = NewObject<UPBEquipmentDataAsset>(this);
	Data->ItemName = FText::FromName(ID);
	Data->ItemType = EPBItemType::Weapon;
	Data->MaxStackSize = 1;
	Data->WeaponHandType = EPBWeaponHandType::OneHanded;
	Data->AllowedSlots = {
		EPBEquipSlot::WeaponSet1_Main, EPBEquipSlot::WeaponSet1_Off,
		EPBEquipSlot::WeaponSet2_Main, EPBEquipSlot::WeaponSet2_Off
	};
	CreatedDataAssets.Add(Data);
	return Data;
}

UPBEquipmentDataAsset* APBItemFunctionalTestBase::CreateTwoHandedWeaponData(const FName& ID)
{
	UPBEquipmentDataAsset* Data = NewObject<UPBEquipmentDataAsset>(this);
	Data->ItemName = FText::FromName(ID);
	Data->ItemType = EPBItemType::Weapon;
	Data->MaxStackSize = 1;
	Data->WeaponHandType = EPBWeaponHandType::TwoHanded;
	Data->AllowedSlots = {
		EPBEquipSlot::WeaponSet1_Main,
		EPBEquipSlot::WeaponSet2_Main
	};
	CreatedDataAssets.Add(Data);
	return Data;
}

UPBEquipmentDataAsset* APBItemFunctionalTestBase::CreateArmorData(const FName& ID, EPBEquipSlot Slot)
{
	UPBEquipmentDataAsset* Data = NewObject<UPBEquipmentDataAsset>(this);
	Data->ItemName = FText::FromName(ID);
	Data->ItemType = EPBItemType::Armor;
	Data->MaxStackSize = 1;
	Data->AllowedSlots = { Slot };
	CreatedDataAssets.Add(Data);
	return Data;
}

UPBInventoryComponent* APBItemFunctionalTestBase::GetInventory(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	return Actor->FindComponentByClass<UPBInventoryComponent>();
}

UPBEquipmentComponent* APBItemFunctionalTestBase::GetEquipment(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}
	return Actor->FindComponentByClass<UPBEquipmentComponent>();
}

// ============================================================
// Test 1: 인벤토리 추가/제거
// ============================================================
void APBTest_InventoryAddRemove::StartTest()
{
	Super::StartTest();

	AActor* TestActor = SpawnTestActor();
	UPBInventoryComponent* Inv = GetInventory(TestActor);
	AssertIsValid(Inv, TEXT("InventoryComponent가 존재해야 한다"));

	// --- 단일 아이템 추가 ---
	UPBItemDataAsset* Potion = CreateConsumableData(FName("Potion_Health"), 5);
	int32 Added = Inv->AddItem(Potion, 3);
	AssertEqual_Int(Added, 3, TEXT("3개 추가 성공해야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 1, TEXT("슬롯 1개 사용 중이어야 한다"));

	// --- 스택 합산 ---
	Added = Inv->AddItem(Potion, 2);
	AssertEqual_Int(Added, 2, TEXT("2개 추가로 합산 성공해야 한다"));
	FPBItemInstance Slot0 = Inv->GetItemAtSlot(0);
	AssertEqual_Int(Slot0.Count, 5, TEXT("합산 후 스택 5개여야 한다"));

	// --- 스택 초과 → 새 슬롯 ---
	Added = Inv->AddItem(Potion, 3);
	AssertEqual_Int(Added, 3, TEXT("초과분 3개 새 슬롯에 추가되어야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 2, TEXT("슬롯 2개 사용 중이어야 한다"));

	// --- 부분 제거 ---
	FGuid FirstID = Inv->GetItemAtSlot(0).InstanceID;
	bool bRemoved = Inv->RemoveItem(FirstID, 2);
	AssertTrue(bRemoved, TEXT("부분 제거 성공해야 한다"));
	AssertEqual_Int(Inv->GetItemAtSlot(0).Count, 3, TEXT("부분 제거 후 3개 남아야 한다"));

	// --- 전체 제거 ---
	bRemoved = Inv->RemoveItem(FirstID, 3);
	AssertTrue(bRemoved, TEXT("전체 제거 성공해야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 1, TEXT("슬롯 1개만 남아야 한다"));

	// --- 비스택 아이템 (장비) ---
	UPBEquipmentDataAsset* Sword = CreateOneHandedWeaponData(FName("Sword_Test"));
	Added = Inv->AddItem(Sword, 1);
	AssertEqual_Int(Added, 1, TEXT("장비 1개 추가 성공해야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 2, TEXT("슬롯 2개 사용 중이어야 한다"));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("인벤토리 추가/제거 테스트 통과"));
}

// ============================================================
// Test 2: 장비 장착/해제
// ============================================================
void APBTest_EquipUnequip::StartTest()
{
	Super::StartTest();

	AActor* TestActor = SpawnTestActor();
	UPBInventoryComponent* Inv = GetInventory(TestActor);
	UPBEquipmentComponent* Equip = GetEquipment(TestActor);
	AssertIsValid(Inv, TEXT("InventoryComponent가 존재해야 한다"));
	AssertIsValid(Equip, TEXT("EquipmentComponent가 존재해야 한다"));

	// --- 장착 ---
	UPBEquipmentDataAsset* Sword = CreateOneHandedWeaponData(FName("Sword_Equip"));
	Inv->AddItem(Sword, 1);
	FGuid SwordID = Inv->GetItemAtSlot(0).InstanceID;

	bool bEquipped = Equip->EquipItem(SwordID, EPBEquipSlot::WeaponSet1_Main, Inv);
	AssertTrue(bEquipped, TEXT("장착 성공해야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 0, TEXT("인벤토리에서 제거되어야 한다"));
	AssertFalse(Equip->IsSlotEmpty(EPBEquipSlot::WeaponSet1_Main), TEXT("슬롯에 아이템이 있어야 한다"));

	// --- 해제 ---
	bool bUnequipped = Equip->UnequipItem(EPBEquipSlot::WeaponSet1_Main, Inv);
	AssertTrue(bUnequipped, TEXT("해제 성공해야 한다"));
	AssertTrue(Equip->IsSlotEmpty(EPBEquipSlot::WeaponSet1_Main), TEXT("슬롯이 비어야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 1, TEXT("인벤토리에 복귀해야 한다"));

	// --- 비허용 슬롯 거부 ---
	UPBEquipmentDataAsset* Helmet = CreateArmorData(FName("Helmet_Test"), EPBEquipSlot::Head);
	Inv->AddItem(Helmet, 1);
	FGuid HelmetID = Inv->GetItemAtSlot(1).InstanceID;

	bool bWrongSlot = Equip->EquipItem(HelmetID, EPBEquipSlot::Body, Inv);
	AssertFalse(bWrongSlot, TEXT("비허용 슬롯 장착은 거부되어야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 2, TEXT("거부 시 인벤토리 변화 없어야 한다"));

	// --- 정상 슬롯 장착 ---
	bool bHeadEquip = Equip->EquipItem(HelmetID, EPBEquipSlot::Head, Inv);
	AssertTrue(bHeadEquip, TEXT("허용 슬롯 장착은 성공해야 한다"));

	// --- 기존 장비 교체 ---
	UPBEquipmentDataAsset* Helmet2 = CreateArmorData(FName("Helmet_Test2"), EPBEquipSlot::Head);
	Inv->AddItem(Helmet2, 1);
	FGuid Helmet2ID = Inv->GetItemAtSlot(1).InstanceID;

	bool bSwap = Equip->EquipItem(Helmet2ID, EPBEquipSlot::Head, Inv);
	AssertTrue(bSwap, TEXT("교체 장착 성공해야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 2, TEXT("교체 시 기존 장비가 인벤토리로 복귀해야 한다"));

	FPBItemInstance HeadItem = Equip->GetEquippedItem(EPBEquipSlot::Head);
	AssertTrue(HeadItem.ItemDataAsset == Helmet2, TEXT("새 투구가 장착되어 있어야 한다"));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("장비 장착/해제 테스트 통과"));
}

// ============================================================
// Test 3: 양손무기 장착 규칙
// ============================================================
void APBTest_TwoHandedWeapon::StartTest()
{
	Super::StartTest();

	AActor* TestActor = SpawnTestActor();
	UPBInventoryComponent* Inv = GetInventory(TestActor);
	UPBEquipmentComponent* Equip = GetEquipment(TestActor);

	UPBEquipmentDataAsset* Greatsword = CreateTwoHandedWeaponData(FName("Greatsword"));
	UPBEquipmentDataAsset* Shield = CreateOneHandedWeaponData(FName("Shield"));
	UPBEquipmentDataAsset* Dagger = CreateOneHandedWeaponData(FName("Dagger"));

	// --- Off 슬롯에 먼저 장착 ---
	Inv->AddItem(Shield, 1);
	FGuid ShieldID = Inv->GetItemAtSlot(0).InstanceID;
	Equip->EquipItem(ShieldID, EPBEquipSlot::WeaponSet1_Off, Inv);
	AssertFalse(Equip->IsSlotEmpty(EPBEquipSlot::WeaponSet1_Off), TEXT("Off에 방패 장착되어야 한다"));

	// --- 양손무기 Main 장착 → Off 자동 해제 ---
	Inv->AddItem(Greatsword, 1);
	FGuid GreatswordID = Inv->GetItemAtSlot(0).InstanceID;
	bool bEquipped = Equip->EquipItem(GreatswordID, EPBEquipSlot::WeaponSet1_Main, Inv);
	AssertTrue(bEquipped, TEXT("양손무기 Main 장착 성공해야 한다"));
	AssertTrue(Equip->IsSlotEmpty(EPBEquipSlot::WeaponSet1_Off), TEXT("양손무기 장착 시 Off 자동 해제되어야 한다"));
	AssertEqual_Int(Inv->GetUsedSlotCount(), 1, TEXT("방패가 인벤토리로 복귀해야 한다"));

	// --- 양손무기 Off 슬롯 장착 거부 ---
	Inv->AddItem(Greatsword, 1);
	FGuid Greatsword2ID = Inv->GetItemAtSlot(1).InstanceID;
	bool bOffFail = Equip->EquipItem(Greatsword2ID, EPBEquipSlot::WeaponSet1_Off, Inv);
	AssertFalse(bOffFail, TEXT("양손무기 Off 슬롯 장착은 거부되어야 한다"));

	// --- Main에 양손무기 있을 때 Off 장착 거부 ---
	Inv->AddItem(Dagger, 1);
	FGuid DaggerID = Inv->GetItemAtSlot(2).InstanceID;
	bool bOffBlock = Equip->EquipItem(DaggerID, EPBEquipSlot::WeaponSet1_Off, Inv);
	AssertFalse(bOffBlock, TEXT("Main에 양손무기 있으면 Off 장착 거부되어야 한다"));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("양손무기 장착 규칙 테스트 통과"));
}

// ============================================================
// Test 4: 자동 슬롯 결정
// ============================================================
void APBTest_AutoEquipSlot::StartTest()
{
	Super::StartTest();

	AActor* TestActor = SpawnTestActor();
	UPBInventoryComponent* Inv = GetInventory(TestActor);
	UPBEquipmentComponent* Equip = GetEquipment(TestActor);

	// --- 단일 슬롯 방어구 → Head 자동 결정 ---
	UPBEquipmentDataAsset* Helmet = CreateArmorData(FName("Helmet_Auto"), EPBEquipSlot::Head);
	AssertTrue(Equip->ResolveAutoEquipSlot(Helmet) == EPBEquipSlot::Head,
		TEXT("투구는 Head로 자동 결정되어야 한다"));

	// --- 한손무기 → 활성 세트 Main 우선 ---
	UPBEquipmentDataAsset* Sword = CreateOneHandedWeaponData(FName("Sword_Auto"));
	AssertTrue(Equip->ResolveAutoEquipSlot(Sword) == EPBEquipSlot::WeaponSet1_Main,
		TEXT("한손무기는 활성 세트 Main으로 결정되어야 한다"));

	// Main 점유 후 → Off
	Inv->AddItem(Sword, 1);
	FGuid SwordID = Inv->GetItemAtSlot(0).InstanceID;
	Equip->EquipItem(SwordID, EPBEquipSlot::WeaponSet1_Main, Inv);

	UPBEquipmentDataAsset* Sword2 = CreateOneHandedWeaponData(FName("Sword_Auto2"));
	AssertTrue(Equip->ResolveAutoEquipSlot(Sword2) == EPBEquipSlot::WeaponSet1_Off,
		TEXT("Main 점유 시 Off로 결정되어야 한다"));

	// Off도 점유 → Main 교체
	Inv->AddItem(Sword2, 1);
	FGuid Sword2ID = Inv->GetItemAtSlot(0).InstanceID;
	Equip->EquipItem(Sword2ID, EPBEquipSlot::WeaponSet1_Off, Inv);

	UPBEquipmentDataAsset* Sword3 = CreateOneHandedWeaponData(FName("Sword_Auto3"));
	AssertTrue(Equip->ResolveAutoEquipSlot(Sword3) == EPBEquipSlot::WeaponSet1_Main,
		TEXT("둘 다 점유 시 Main 교체로 결정되어야 한다"));

	// --- 양손무기 → Main 고정 ---
	UPBEquipmentDataAsset* Greatsword = CreateTwoHandedWeaponData(FName("Greatsword_Auto"));
	AssertTrue(Equip->ResolveAutoEquipSlot(Greatsword) == EPBEquipSlot::WeaponSet1_Main,
		TEXT("양손무기는 Main으로 결정되어야 한다"));

	// --- AutoEquipItem 통합 테스트 ---
	Inv->AddItem(Helmet, 1);
	FGuid HelmetID = Inv->GetItemAtSlot(0).InstanceID;
	bool bAutoEquip = Equip->AutoEquipItem(HelmetID, Inv);
	AssertTrue(bAutoEquip, TEXT("AutoEquipItem 성공해야 한다"));
	AssertFalse(Equip->IsSlotEmpty(EPBEquipSlot::Head), TEXT("투구가 Head에 장착되어야 한다"));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("자동 슬롯 결정 테스트 통과"));
}

// ============================================================
// Test 5: 무기 세트 전환
// ============================================================
void APBTest_WeaponSetSwitch::StartTest()
{
	Super::StartTest();

	AActor* TestActor = SpawnTestActor();
	UPBInventoryComponent* Inv = GetInventory(TestActor);
	UPBEquipmentComponent* Equip = GetEquipment(TestActor);

	// 초기 활성 세트 확인
	AssertEqual_Int(Equip->GetActiveWeaponSet(), 1, TEXT("초기 활성 세트는 1이어야 한다"));

	// 세트1에 무기 장착
	UPBEquipmentDataAsset* Sword = CreateOneHandedWeaponData(FName("Sword_Set1"));
	Inv->AddItem(Sword, 1);
	FGuid SwordID = Inv->GetItemAtSlot(0).InstanceID;
	Equip->EquipItem(SwordID, EPBEquipSlot::WeaponSet1_Main, Inv);

	// 세트2에 무기 장착
	UPBEquipmentDataAsset* Bow = CreateOneHandedWeaponData(FName("Bow_Set2"));
	Inv->AddItem(Bow, 1);
	FGuid BowID = Inv->GetItemAtSlot(0).InstanceID;
	Equip->EquipItem(BowID, EPBEquipSlot::WeaponSet2_Main, Inv);

	// 양쪽 세트 모두 장비 유지 확인
	AssertFalse(Equip->IsSlotEmpty(EPBEquipSlot::WeaponSet1_Main), TEXT("세트1 Main에 장비 있어야 한다"));
	AssertFalse(Equip->IsSlotEmpty(EPBEquipSlot::WeaponSet2_Main), TEXT("세트2 Main에 장비 있어야 한다"));

	// 세트 전환 1→2
	Equip->SwitchWeaponSet();
	AssertEqual_Int(Equip->GetActiveWeaponSet(), 2, TEXT("전환 후 활성 세트 2여야 한다"));

	// 장비는 양쪽 다 유지
	AssertFalse(Equip->IsSlotEmpty(EPBEquipSlot::WeaponSet1_Main), TEXT("전환 후에도 세트1 장비 유지"));
	AssertFalse(Equip->IsSlotEmpty(EPBEquipSlot::WeaponSet2_Main), TEXT("전환 후에도 세트2 장비 유지"));

	// 재전환 2→1
	Equip->SwitchWeaponSet();
	AssertEqual_Int(Equip->GetActiveWeaponSet(), 1, TEXT("재전환 후 활성 세트 1이어야 한다"));

	FinishTest(EFunctionalTestResult::Succeeded, TEXT("무기 세트 전환 테스트 통과"));
}
