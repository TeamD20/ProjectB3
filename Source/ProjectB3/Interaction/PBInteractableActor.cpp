// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBInteractableActor.h"

#include "PBInteractableComponent.h"


// Sets default values
APBInteractableActor::APBInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractableComponent = CreateDefaultSubobject<UPBInteractableComponent>(TEXT("InteractableComponent"));
}
