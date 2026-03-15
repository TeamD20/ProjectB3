// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBItemFunctionalTest.generated.h"

class UPBInventoryComponent;
class UPBEquipmentComponent;
class UPBItemDataAsset;
class UPBEquipmentDataAsset;

/**
 * 아이템 시스템 Functional Test 기반 클래스.
 * 더미 DataAsset 생성 및 컴포넌트 부착 헬퍼를 제공한다.
 */
UCLASS()
class PROJECTB3_API APBItemFunctionalTestBase : public AFunctionalTest
{
	GENERATED_BODY()

public:
	APBItemFunctionalTestBase();

protected:
	virtual void PrepareTest() override;
	virtual void CleanUp() override;

	// 인벤토리 + 장비 컴포넌트를 가진 더미 액터 생성
	AActor* SpawnTestActor();

	// 소모품용 더미 DataAsset 생성 (스택 가능)
	UPBItemDataAsset* CreateConsumableData(const FName& ID, int32 MaxStack = 5);

	// 한손 무기용 더미 EquipmentDataAsset 생성
	UPBEquipmentDataAsset* CreateOneHandedWeaponData(const FName& ID);

	// 양손 무기용 더미 EquipmentDataAsset 생성
	UPBEquipmentDataAsset* CreateTwoHandedWeaponData(const FName& ID);

	// 방어구용 더미 EquipmentDataAsset 생성
	UPBEquipmentDataAsset* CreateArmorData(const FName& ID, EPBEquipmentType EquipType);

	// 테스트 액터에서 컴포넌트 획득
	UPBInventoryComponent* GetInventory(AActor* Actor) const;
	UPBEquipmentComponent* GetEquipment(AActor* Actor) const;

	// 스폰된 액터 목록 (CleanUp용)
	UPROPERTY()
	TArray<TObjectPtr<AActor>> SpawnedActors;

	// 생성된 DataAsset 목록 (GC 방지용)
	UPROPERTY()
	TArray<TObjectPtr<UPBItemDataAsset>> CreatedDataAssets;
};

/**
 * 인벤토리 추가/제거 기본 테스트
 * - 단일 아이템 추가 및 조회
 * - 스택 합산 확인
 * - 아이템 제거 (부분/전체)
 * - 인벤토리 용량 초과 시 미추가 확인
 */
UCLASS()
class PROJECTB3_API APBTest_InventoryAddRemove : public APBItemFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 장비 장착/해제 기본 테스트
 * - 인벤토리 → 장비 슬롯 장착
 * - 장비 슬롯 → 인벤토리 해제
 * - 슬롯 호환성 검증 (비허용 슬롯 거부)
 * - 기존 장비 자동 교체
 */
UCLASS()
class PROJECTB3_API APBTest_EquipUnequip : public APBItemFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 양손무기 장착 규칙 테스트
 * - 양손무기 Main 장착 시 Off 자동 해제
 * - 양손무기 Off 슬롯 장착 거부
 * - Main에 양손무기 있을 때 Off 장착 거부
 */
UCLASS()
class PROJECTB3_API APBTest_TwoHandedWeapon : public APBItemFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 자동 슬롯 결정 테스트
 * - 단일 슬롯 장비 자동 결정
 * - 한손무기 Main 우선 → Off 차선
 * - 양손무기 Main 고정
 * - 슬롯 모두 점유 시 Main 교체
 */
UCLASS()
class PROJECTB3_API APBTest_AutoEquipSlot : public APBItemFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};

/**
 * 무기 세트 전환 테스트
 * - 세트1 장착 상태에서 세트2로 전환
 * - 전환 후 활성 세트 번호 확인
 * - 전환 후 재전환 (1→2→1)
 */
UCLASS()
class PROJECTB3_API APBTest_WeaponSetSwitch : public APBItemFunctionalTestBase
{
	GENERATED_BODY()

protected:
	virtual void StartTest() override;
};
