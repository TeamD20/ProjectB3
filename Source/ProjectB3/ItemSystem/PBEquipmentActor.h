// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ProjectB3/Game/PBPrewarmInterface.h"
#include "PBEquipmentActor.generated.h"


// 장비 액터, 필드 드랍 혹은 캐릭터 메시에 부착
UCLASS()
class PROJECTB3_API APBEquipmentActor : public AActor, public IPBPrewarmInterface
{
	GENERATED_BODY()

public:
	APBEquipmentActor();
	
	// 장비 애니메이션 레이어 활성화
	void LinkAnimLayer(USkeletalMeshComponent* InMeshComponent) const;

	// 장비 애니메이션 레이어 해제
	void UnlinkAnimLayer(USkeletalMeshComponent* InMeshComponent) const;

	// 장착 애니메이션 조회
	UAnimMontage* GetEquipAnimMontage() const {return EquipAnimMontage;};
	
	// 장착 해제 애니메이션 조회
	UAnimMontage* GetUnEquipAnimMontage() const {return UnEquipAnimMontage;};
	
	// 투사체 발사 기점 Transform 반환 (ProjectileLaunchSocket 기준)
	UFUNCTION(BlueprintNativeEvent, Category = "Equipment|Projectile")
	FTransform GetProjectileLaunchTransform() const;

protected:
	// 애니메이션 레이어 클래스
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Equipment")
	TSubclassOf<UAnimInstance> AnimLayerClass;

	// 장착 애니메이션 (Optional)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Equipment")
	TObjectPtr<UAnimMontage> EquipAnimMontage;
	
	// 해제 애니메이션 (Optional)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Equipment")
	TObjectPtr<UAnimMontage> UnEquipAnimMontage;
	
	// 장비 스태틱 메시 컴포넌트 (소켓 활용 목적)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UStaticMeshComponent> EquipmentMesh;
};