// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBItemDataAsset.generated.h"

// 아이템 마스터 DataAsset — 기본 클래스
// DA_Item_HealthPotion, DA_Item_Longsword 등
UCLASS(BlueprintType)
class PROJECTB3_API UPBItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// UI상 표시할 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText ItemName;

	// UI 슬롯 아이콘 텍스처
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	// 아이템 대분류
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EPBItemType ItemType = EPBItemType::Misc;

	// 등급 (UI 테두리 색상)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EPBItemRarity Rarity = EPBItemRarity::Common;

	// 어빌리티/효과 설명 텍스트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Description", meta = (MultiLine = true))
	FText ItemDescription;

	// 배경 서사/로어 텍스트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Description", meta = (MultiLine = true))
	FText BackGroundDescription;

	// 겹치기 최대 수량 (장비는 1, 소모품은 N)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	// 필드 드롭 시 월드에 표시할 스태틱 메시 (없으면 FieldDropActor 기본 메시 사용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|WorldDrop")
	TObjectPtr<UStaticMesh> DropMesh;

	// --- 소모품 전용 필드 (ItemType == Consumable 일 때만 에디터에 표시) ---

	// 사용 시 즉각적인 효과 설명 (예: "3d4+3 회복")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "ItemType == EPBItemType::Consumable", EditConditionHides))
	FText EffectText;

	// 효과 설명 앞 아이콘 (물약, 스크롤, 체력 아이콘 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "ItemType == EPBItemType::Consumable", EditConditionHides))
	TSoftObjectPtr<UTexture2D> EffectIcon;

	// 지속시간 또는 소모성 텍스트 (예: "긴 휴식 전까지", "사용 시 1개 소모")
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (EditCondition = "ItemType == EPBItemType::Consumable", EditConditionHides))
	FText DurationText;

public:
	// 스택 가능 여부
	bool IsStackable() const { return MaxStackSize > 1; }

	/*~ UPrimaryDataAsset Interface ~*/
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
