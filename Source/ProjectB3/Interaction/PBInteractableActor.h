// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (상호작용 액터 기본 동작 작성)

#pragma once

#include "CoreMinimal.h"
#include "PBInteractionInterface.h"
#include "GameFramework/Actor.h"
#include "PBInteractableActor.generated.h"

UCLASS()
class PROJECTB3_API APBInteractableActor : public AActor, public IPBInteractionInterface
{
	GENERATED_BODY()

public:
	APBInteractableActor();
	
	virtual UPBInteractableComponent* GetInteractableComponent() const override {return InteractableComponent;}
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPBInteractableComponent> InteractableComponent;
};
