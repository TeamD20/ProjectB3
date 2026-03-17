// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBInteractionAction.generated.h"

class UPBInteractableComponent;
class AController;
class APawn;

/**
 * 상호작용 행동 베이스 클래스.
 * UPBInteractableComponent에 추가되며, OnInteract 시 조건에 맞는 최우선 행동이 실행된다.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, Abstract)
class PROJECTB3_API UPBInteractionAction : public UObject
{
	GENERATED_BODY()

public:
	/** Interactor(PlayerController)로부터 빙의 폰 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	APawn* GetPawn(const AActor* Interactor) const;

	/** Interactor(PlayerController)로부터 컨트롤러 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	AController* GetController(const AActor* Interactor) const;

	/** 이 행동이 현재 실행 가능한지 여부 반환 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const;

	/** 상호작용 행동 실행 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Execute(AActor* Interactor);
	virtual void Execute_Implementation(AActor* Interactor);

	/** 우선순위 반환. 값이 높을수록 먼저 선택된다 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	int32 GetPriority() const;
	virtual int32 GetPriority_Implementation() const;

public:
	// 이 행동의 기본 우선순위. 하위 클래스에서 재정의 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	int32 Priority = 0;
};
