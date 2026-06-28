// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (장비 아이템 월드 액터 표현)


#include "PBEquipmentActor.h"
#include "Components/StaticMeshComponent.h"

APBEquipmentActor::APBEquipmentActor()
{
	EquipmentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquipmentMesh"));
	SetRootComponent(EquipmentMesh);
}

void APBEquipmentActor::LinkAnimLayer(USkeletalMeshComponent* InMeshComponent) const
{
	if (!IsValid(InMeshComponent) || !IsValid(AnimLayerClass))
	{
		return;
	}

	InMeshComponent->LinkAnimClassLayers(AnimLayerClass);
}

void APBEquipmentActor::UnlinkAnimLayer(USkeletalMeshComponent* InMeshComponent) const
{
	if (!IsValid(InMeshComponent) || !IsValid(AnimLayerClass))
	{
		return;
	}

	InMeshComponent->UnlinkAnimClassLayers(AnimLayerClass);
}

FTransform APBEquipmentActor::GetProjectileLaunchTransform_Implementation() const
{
	if (EquipmentMesh)
	{
		return EquipmentMesh->GetComponentTransform();
	}
	return FTransform();
}
