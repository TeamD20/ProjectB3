// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectB3/PBGameplayTags.h"
#include "ProjectB3/Interaction/PBInteractionAction.h"
#include "PBInteraction_LootAction.generated.h"

class UPBLootPanelWidget;
class UPBInteractorComponent;

/**
 * 루팅 상호작용 행동.
 * Owner의 ASC가 있으면 LootableTag 보유 여부를 확인하고,
 * ASC가 없으면 항상 루팅 가능하다.
 * 조건 충족 시 루팅 UI를 Push하고, 유지형 상호작용으로 추적한다.
 */
UCLASS()
class PROJECTB3_API UPBInteraction_LootAction : public UPBInteractionAction
{
	GENERATED_BODY()

public:
	UPBInteraction_LootAction();

	/*~ UPBInteractionAction Interface ~*/
	/** Owner의 ASC 유무 및 LootableTag 보유 여부로 루팅 가능 여부 반환 */
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	/** 루팅 UI를 Push하고 대상 액터로 초기화. OnLootClosed 델리게이트 바인딩 */
	virtual void Execute_Implementation(AActor* Interactor) override;

	/** 거리 초과 등 외부 종료 시 호출. 위젯이 열려있으면 Pop */
	virtual void EndInteraction_Implementation() override;

public:
	// ASC가 있는 대상에게 요구하는 루팅 가능 상태 태그
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	FGameplayTag LootableTag = PBGameplayTags::Character_State_Lootable;

	// Push할 루팅 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<UPBLootPanelWidget> LootWidgetClass;

private:
	/** 위젯의 OnLootClosed 델리게이트 핸들러. InteractorComponent에 종료 요청 */
	UFUNCTION()
	void HandleLootWidgetClosed();

private:
	// 캐시된 루팅 위젯 (약참조)
	TWeakObjectPtr<UPBLootPanelWidget> CachedLootWidget;

	// 캐시된 InteractorComponent (종료 요청용)
	TWeakObjectPtr<UPBInteractorComponent> CachedInteractorComponent;
};
