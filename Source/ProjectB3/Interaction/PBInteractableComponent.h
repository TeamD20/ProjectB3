// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PBInteractableComponent.generated.h"

class UPBInteractionAction;
class UPBInteractorComponent;
class UMeshComponent;

/**
 * 상호작용 가능한 오브젝트에 부착하는 컴포넌트.
 * UPBInteractionAction 목록을 보유하며, OnInteract 시 조건을 만족하는
 * 가장 높은 우선순위의 행동을 선택하여 실행한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTB3_API UPBInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBInteractableComponent();
	
	/** 포커스 진입 시 호출. 소유 액터의 모든 메시에 오버레이 머티리얼 적용 */
	virtual void OnFocus(bool bIsInRange);

	/** 포커스 해제 시 호출. 소유 액터의 모든 메시에서 오버레이 머티리얼 제거 */
	virtual void OnUnfocus();

	/** 상호작용 시 호출. 조건을 만족하는 최고 우선순위 행동을 실행한다 */
	virtual void OnInteract(AActor* Interactor);

	/** 실행 가능한 행동이 하나라도 있는지 여부 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool HasAvailableAction(AActor* Interactor) const;

	/** 활성 유지형 상호작용을 종료한다 */
	void EndActiveInteraction();

	/** 현재 활성 Action 반환 (nullptr이면 비활성) */
	UPBInteractionAction* GetActiveAction() const { return ActiveAction; }

public:
	// 이 컴포넌트에 등록된 상호작용 행동 목록
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "Interaction")
	TArray<TObjectPtr<UPBInteractionAction>> InteractionActions;

private:
	// 현재 포커스 상태 여부
	bool bIsFocused = false;

	// 포커스 진입 전 메시별 RenderCustomDepth 원래 값 (복원용)
	UPROPERTY()
	TMap<TObjectPtr<UMeshComponent>, bool> SavedCustomDepthStates;

	// 현재 활성 유지형 Action
	UPROPERTY()
	TObjectPtr<UPBInteractionAction> ActiveAction;
};