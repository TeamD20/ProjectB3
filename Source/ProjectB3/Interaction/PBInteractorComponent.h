// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PBInteractorComponent.generated.h"

class IPBInteractionInterface;

class UPBInteractableComponent;

/**
 * 상호작용 주체(플레이어)에 부착하는 컴포넌트.
 * 한 번에 하나의 UPBInteractableComponent만 포커스로 관리하며,
 * 상호작용 요청을 현재 포커스 대상에 위임한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTB3_API UPBInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPBInteractorComponent();

	/**
	 * 액터로부터 포커스 대상을 설정한다.
	 * IPBInteractionInterface를 구현하지 않은 경우 기존 포커스도 해제된다.
	 */
	void TryFocus(AActor* Actor);

	/** 포커스 대상을 변경한다. 기존 대상에 OnUnfocus, 신규 대상에 OnFocus 호출 */
	void SetFocus(UPBInteractableComponent* NewFocus);

	/** 포커스를 해제한다 */
	void ClearFocus();

	/** 현재 포커스 대상에 상호작용을 위임한다 */
	void Interact();

	/** 현재 포커스 대상 반환 */
	UPBInteractableComponent* GetFocusedComponent() const { return FocusedComponent; }

	/** 현재 유효한 포커스 대상이 있는지 여부 반환 */
	bool HasFocus() const;

	/** 대상 액터가 상호작용 최대 거리 이내인지 여부 반환 */
	bool IsWithinRange(const AActor* Target) const;

	/** 포커스 대상에서 제외할 액터 목록 설정 */
	void SetIgnoreActors(const TArray<AActor*>& Actors);

public:
	// 상호작용 최대 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float MaxInteractionDistance = 200.f;

private:
	// 현재 포커스 중인 상호작용 컴포넌트
	UPROPERTY()
	TObjectPtr<UPBInteractableComponent> FocusedComponent;

	// 포커스에서 제외할 액터 목록
	TArray<TWeakObjectPtr<AActor>> IgnoreActors;
};
