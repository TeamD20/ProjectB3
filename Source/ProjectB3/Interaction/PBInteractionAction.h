// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PBInteractionAction.generated.h"

class UPBInteractableComponent;
class AController;
class APawn;

/**
 * 상호작용 유형.
 * Instant: 즉발형 — Execute 한 번 호출로 완료. 추적 불필요 (기본값)
 * Sustained: 유지형 — Execute로 시작, EndInteraction으로 종료. 활성 상태 추적 대상
 */
UENUM(BlueprintType)
enum class EPBInteractionType : uint8
{
	// 즉발형: Execute 한 번 호출로 완료
	Instant,

	// 유지형: Execute로 시작, EndInteraction으로 종료
	Sustained,
};

/**
 * 상호작용 행동 베이스 클래스.
 * UPBInteractableComponent에 추가되며, OnInteract 시 조건에 맞는 최우선 행동이 실행된다.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, Abstract)
class PROJECTB3_API UPBInteractionAction : public UObject
{
	GENERATED_BODY()

public:
	/** 상호작용 대상 (InteractableComponent 소유자) 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	AActor* GetOwner() const;

	/** Interactor로부터 Interactor Pawn 반환 */
	APawn* GetPawn(AActor* Interactor) const;
	
	/** Interactor로부터 Interactor Controller 반환 */
	AController* GetController(AActor* Interactor) const;
	
	/** 이 행동이 현재 실행 가능한지 여부 반환 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;
	virtual bool CanInteract_Implementation(AActor* Interactor) const;

	/** 상호작용 행동 실행. Sustained인 경우 bIsActive를 true로 설정 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Execute(AActor* Interactor);
	virtual void Execute_Implementation(AActor* Interactor);

	/** 유지형 상호작용 종료. 하위 클래스에서 정리 로직 override */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void EndInteraction();
	virtual void EndInteraction_Implementation();

	/** 우선순위 반환. 값이 높을수록 먼저 선택된다 */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	int32 GetPriority() const;
	virtual int32 GetPriority_Implementation() const;

	/** 현재 상호작용 활성 상태 여부 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool IsActive() const { return bIsActive; }

	/** InteractionType == Sustained 여부 반환 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool IsSustained() const { return InteractionType == EPBInteractionType::Sustained; }

public:
	// 이 행동의 기본 우선순위. 하위 클래스에서 재정의 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	int32 Priority = 0;

	// 상호작용 유형 (즉발형/유지형)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	EPBInteractionType InteractionType = EPBInteractionType::Instant;

	// 거리 유지 필요 여부. true면 InteractorComponent가 Tick에서 거리 체크
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
	bool bRequiresRange = false;

protected:
	// 현재 상호작용 활성 상태 (유지형 전용)
	bool bIsActive = false;
};
