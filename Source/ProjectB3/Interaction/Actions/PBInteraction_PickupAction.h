// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/Interaction/PBInteractionAction.h"
#include "PBInteraction_PickupAction.generated.h"

/**
 * 필드 드롭 아이템 줍기 상호작용 액션.
 * 줍기 성공 시 아이템 인스턴스를 인벤토리로 이전하고 FieldDropActor를 제거한다.
 */
UCLASS()
class PROJECTB3_API UPBInteraction_PickupAction : public UPBInteractionAction
{
	GENERATED_BODY()

public:
	UPBInteraction_PickupAction();

	/*~ UPBInteractionAction Interface ~*/
	// Interactor가 InventoryComponent를 보유하고 빈 슬롯이 남아있을 때 줍기 가능
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	// 아이템 인스턴스를 인벤토리로 이전하고 FieldDropActor를 Destroy
	virtual void Execute_Implementation(AActor* Interactor) override;
};
