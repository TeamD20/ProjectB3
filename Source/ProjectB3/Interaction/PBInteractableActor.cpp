// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (상호작용 액터 기본 동작 작성)


#include "PBInteractableActor.h"

#include "PBInteractableComponent.h"


// Sets default values
APBInteractableActor::APBInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractableComponent = CreateDefaultSubobject<UPBInteractableComponent>(TEXT("InteractableComponent"));
}
