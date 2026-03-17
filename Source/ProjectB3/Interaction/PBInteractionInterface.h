// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PBInteractionInterface.generated.h"

class UPBInteractableComponent;
// This class does not need to be modified.
UINTERFACE()
class UPBInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTB3_API IPBInteractionInterface
{
	GENERATED_BODY()

public:
	virtual UPBInteractableComponent* GetInteractableComponent() const = 0;
};