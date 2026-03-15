// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PBEquipmentActor.generated.h"

// 장비 액터, 필드 드랍 혹은 캐릭터 메시에 부착
UCLASS()
class PROJECTB3_API APBEquipmentActor : public AActor
{
	GENERATED_BODY()

public:
	APBEquipmentActor();
	
	// 장비 애니메이션 레이어 활성화
	void LinkAnimLayer(USkeletalMeshComponent* MeshComponent) const;
	
	// 장비 애니메이션 레이어 해제
	void UnlinkAnimLayer(USkeletalMeshComponent* MeshComponent) const;
	
protected:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TSubclassOf<UAnimInstance> AnimLayerClass;
};