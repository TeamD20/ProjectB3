// Copyright (c) 2026 TeamD20. All Rights Reserved.


#include "PBEquipmentActor.h"


// Sets default values
APBEquipmentActor::APBEquipmentActor()
{
}

void APBEquipmentActor::LinkAnimLayer(USkeletalMeshComponent* MeshComponent) const
{
	if (!IsValid(MeshComponent) || !IsValid(AnimLayerClass))
	{
		return;
	}
	
	MeshComponent->LinkAnimClassLayers(AnimLayerClass);
}

void APBEquipmentActor::UnlinkAnimLayer(USkeletalMeshComponent* MeshComponent) const
{
	if (!IsValid(MeshComponent) || !IsValid(AnimLayerClass))
	{
		return;
	}
	
	MeshComponent->UnlinkAnimClassLayers(AnimLayerClass);
}
