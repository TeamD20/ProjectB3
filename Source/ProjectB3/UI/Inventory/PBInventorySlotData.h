// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBInventorySlotData.generated.h"

class UTexture2D;

/** 인벤토리/장비 슬롯 1개의 UI 표시용 스냅샷 데이터 */
USTRUCT(BlueprintType)
struct PROJECTB3_API FPBInventorySlotData
{
	GENERATED_BODY()

	// 아이템 존재 여부
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsEmpty = true;

	// 아이콘 리소스 (약참조)
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TSoftObjectPtr<UTexture2D> ItemIcon;

	// 아이템 이름 (툴팁용)
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText ItemName;

	// 표기 수량 (2 이상일 때만 표시)
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 StackCount = 0;

	// 등급 기반 테두리 색상
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FLinearColor RarityColor = FLinearColor::White;

	// 박스 1 배경 오버레이 색상 (등급 동기화 툴팁 표출용)
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FLinearColor RarityOverlayColor = FLinearColor::Transparent;

	// 상호작용 시 원본 식별용 (드래그, 장착 등)
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid InstanceID;

	// 컨텍스트 메뉴 액션 분기용 아이템 타입
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	EPBItemType ItemType = EPBItemType::Misc;
};
