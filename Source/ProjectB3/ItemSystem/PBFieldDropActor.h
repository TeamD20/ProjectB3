// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "PBFieldDropActor.generated.h"

class UStaticMeshComponent;
class UPBInteractableComponent;

/**
 * 필드에 드롭된 아이템을 표현하는 액터.
 * UPBInteractableComponent + UPBInteraction_PickupAction으로 줍기 상호작용을 처리한다.
 * InitializeDrop으로 아이템 인스턴스를 주입하며,
 * 플레이어가 줍는 순간 PickupAction이 Destroy()를 호출한다.
 */
UCLASS()
class PROJECTB3_API APBFieldDropActor : public AActor
{
	GENERATED_BODY()

public:
	APBFieldDropActor();

	// 드롭 아이템 인스턴스를 설정하고 메시를 갱신한다
	void InitializeDrop(const FPBItemInstance& InItemInstance);

	// 드롭된 아이템 인스턴스 반환
	const FPBItemInstance& GetDroppedItem() const { return DroppedItem; }

protected:
	/*~ AActor Interface ~*/
	virtual void BeginPlay() override;

private:
	// 아이템 시각화용 스태틱 메시 컴포넌트 (루트)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FieldDrop", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	// 줍기 상호작용을 처리하는 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FieldDrop", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPBInteractableComponent> InteractableComponent;

	// 이 액터가 표현하는 아이템 인스턴스
	UPROPERTY(VisibleAnywhere, Category = "FieldDrop")
	FPBItemInstance DroppedItem;
};
