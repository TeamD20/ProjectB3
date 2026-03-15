// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBItemTypes.generated.h"

class UPBItemDataAsset;

// 아이템 대분류
UENUM(BlueprintType)
enum class EPBItemType : uint8
{
	// 무기류
	Weapon,
	// 방어구류 (투구, 갑옷, 장갑, 신발 등)
	Armor,
	// 장신구 (목걸이, 반지)
	Trinket,
	// 소모품 (포션, 스크롤 등 스택 가능)
	Consumable,
	// 기타 (잡동사니, 전리품 등)
	Misc
};

// 아이템 등급
UENUM(BlueprintType)
enum class EPBItemRarity : uint8
{
	// 일반 (테두리 없음)
	Common,
	// 고급 (초록색 테두리)
	Uncommon,
	// 희귀 (파란색 테두리)
	Rare,
	// 전설 (주황/금색 테두리)
	Legendary
};

// 장착 슬롯 부위
UENUM(BlueprintType)
enum class EPBEquipSlot : uint8
{
	// 방어구
	Head,
	Body,
	Hands,
	Feet,

	// 장신구
	Amulet,
	Ring1,
	Ring2,

	// 무기 세트 1 (관례상 근거리)
	WeaponSet1_Main,
	WeaponSet1_Off,

	// 무기 세트 2 (관례상 원거리)
	WeaponSet2_Main,
	WeaponSet2_Off,

	MAX UMETA(Hidden)
};

// 장비 부위 유형 — 에디터에서 지정하면 코드에서 허용 슬롯을 자동 도출
UENUM(BlueprintType)
enum class EPBEquipmentType : uint8
{
	// 무기 (WeaponHandType에 따라 허용 슬롯 결정)
	Weapon,
	// 방패 / 오프핸드 전용 장비
	Shield,
	// 투구
	Head,
	// 갑옷
	Body,
	// 장갑
	Hands,
	// 신발
	Feet,
	// 목걸이
	Amulet,
	// 반지 (Ring1, Ring2 모두 허용)
	Ring,
};

// 무기의 손 점유 유형
UENUM(BlueprintType)
enum class EPBWeaponHandType : uint8
{
	// 한손 무기 (Main 또는 Off 단독 장착)
	OneHanded,
	// 양손 무기 (Main + Off 동시 점유)
	TwoHanded,
	// 다용도 무기 (한손 사용 가능, 양손 사용 시 데미지 증가)
	Versatile
};

// 캐릭터 초기 지급 아이템 항목 (인벤토리용)
USTRUCT(BlueprintType)
struct FPBDefaultItemEntry
{
	GENERATED_BODY()

	// 지급할 아이템 데이터 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPBItemDataAsset> ItemData;

	// 지급 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1"))
	int32 Amount = 1;
};

// 캐릭터 초기 장착 항목 (장비 슬롯용)
USTRUCT(BlueprintType)
struct FPBDefaultEquipmentEntry
{
	GENERATED_BODY()

	// 장착할 아이템 데이터 에셋
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPBItemDataAsset> ItemData;

	// 장착할 슬롯
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPBEquipSlot Slot = EPBEquipSlot::WeaponSet1_Main;
};

// 인벤토리에 보유 중인 개별 아이템 인스턴스
USTRUCT(BlueprintType)
struct FPBItemInstance
{
	GENERATED_BODY()

	// 고유 식별자 (단일 객체 구분용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid InstanceID;

	// 아이템 정의 에셋 참조
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPBItemDataAsset> ItemDataAsset;

	// 현재 누적 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count = 0;

	// 인스턴스 유효성 검증
	bool IsValid() const
	{
		return InstanceID.IsValid() && ItemDataAsset != nullptr && Count > 0;
	}

	// 새 인스턴스 생성 헬퍼
	static FPBItemInstance Create(UPBItemDataAsset* InDataAsset, int32 InCount = 1)
	{
		FPBItemInstance Instance;
		Instance.InstanceID = FGuid::NewGuid();
		Instance.ItemDataAsset = InDataAsset;
		Instance.Count = InCount;
		return Instance;
	}
};
