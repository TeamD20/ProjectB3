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

	// 겹치기 최대 수량 (장비는 1, 소모품은 N)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

public:
	// 스택 가능 여부
	bool IsStackable() const { return MaxStackSize > 1; }

	/*~ UPrimaryDataAsset Interface ~*/
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
