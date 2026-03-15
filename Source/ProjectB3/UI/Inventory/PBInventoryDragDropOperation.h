// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "PBInventoryDragDropOperation.generated.h"

// 인벤토리 슬롯 드래그 전달 데이터
UCLASS()
class PROJECTB3_API UPBInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// 드래그 시작 슬롯의 대상 Actor
	UPROPERTY(BlueprintReadWrite, Category = "Inventory|DragDrop")
	TObjectPtr<AActor> SourceActor = nullptr;

	// 드래그 시작 슬롯 인덱스
	UPROPERTY(BlueprintReadWrite, Category = "Inventory|DragDrop")
	int32 SourceSlotIndex = INDEX_NONE;

	// 드래그한 아이템 인스턴스 ID
	UPROPERTY(BlueprintReadWrite, Category = "Inventory|DragDrop")
	FGuid InstanceID;
};
