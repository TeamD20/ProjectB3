// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBLootPanelWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UPBLootSlotWidget;
class UPBLootViewModel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLootWidgetClosed);

/**
 * 루팅 UI 위젯.
 * InitializeLoot() 호출 시 대상 액터의 인벤토리/장비 슬롯을 단일 패널에 표시하고,
 * 슬롯 클릭으로 아이템을 플레이어 인벤토리로 이동한다.
 */
UCLASS()
class PROJECTB3_API UPBLootPanelWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	/** 루팅 대상 액터와 PlayerController로 위젯을 초기화 */
	void InitializeLoot(AActor* TargetActor, APlayerController* InPlayerController);

	// 위젯이 닫힐 때 Broadcast되는 델리게이트. Action에서 바인딩하여 종료 흐름 처리
	UPROPERTY(BlueprintAssignable, Category = "UI|Loot")
	FOnLootWidgetClosed OnLootClosed;

protected:
	/*~ UUserWidget Interface ~*/
	/** 버튼 이벤트 바인딩 */
	virtual void NativeConstruct() override;

	/** 버튼 이벤트 해제 및 ViewModel 정리 */
	virtual void NativeDestruct() override;

private:
	/** LootViewModel 슬롯 기반으로 슬롯 위젯 재생성 */
	void RebuildSlotWidgets();

	/** 루팅 슬롯 전체 갱신 이벤트 처리 */
	void HandleLootSlotsRefreshed();

	/** 닫기 버튼 클릭 처리 */
	UFUNCTION()
	void OnCloseClicked();

	/** 모두 가져오기 버튼 클릭 처리 */
	UFUNCTION()
	void OnTakeAllClicked();

protected:
	// 대상 이름 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OwnerNameText;

	// 슬롯 위젯들을 담는 패널 (WrapBox 등)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> LootSlotsPanel;

	// 닫기 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// 모두 가져오기 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TakeAllButton;

	// 루팅 슬롯 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Loot")
	TSubclassOf<UPBLootSlotWidget> LootSlotWidgetClass;

private:
	// 현재 활성화된 LootViewModel
	UPROPERTY(Transient)
	TObjectPtr<UPBLootViewModel> LootViewModel;

	// 생성된 슬롯 위젯 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPBLootSlotWidget>> SlotWidgets;

	// LootSlotsRefreshed 델리게이트 핸들
	FDelegateHandle LootSlotsRefreshedHandle;

	// 캐시된 PlayerController
	TWeakObjectPtr<APlayerController> CachedPlayerController;
};
