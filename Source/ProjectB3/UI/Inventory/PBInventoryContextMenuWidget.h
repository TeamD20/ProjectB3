// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBInventoryContextMenuWidget.generated.h"

class UButton;
class UWidget;
class UPBInventoryComponent;
class UPBEquipmentComponent;

/**
 * 인벤토리 슬롯 우클릭 시 표시되는 컨텍스트 메뉴 위젯.
 * AddToViewport로 최상위에 배치되며, 백드롭 버튼으로 외부 클릭 닫힘을 처리한다.
 * 아이템 타입에 따라 장착 / 사용 / 버리기 버튼을 선택적으로 표시한다.
 */
UCLASS()
class PROJECTB3_API UPBInventoryContextMenuWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 컨텍스트 메뉴 초기화 — 각 컴포넌트를 직접 주입하고 버튼 가시성을 결정
	// ScreenPosition: ContentPanel을 배치할 뷰포트 기준 물리 픽셀 좌표
	void Initialize(
		const FGuid& InInstanceID,
		EPBItemType InItemType,
		UPBInventoryComponent* InInventory,
		UPBEquipmentComponent* InEquipment,
		AActor* InTargetActor,
		FVector2D ScreenPosition);

	// 메뉴를 숨기고 내부 상태를 초기화
	void CloseMenu();

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	/*~ 버튼 이벤트 핸들러 ~*/
	UFUNCTION()
	void HandleEquipClicked();

	UFUNCTION()
	void HandleUseClicked();

	UFUNCTION()
	void HandleDiscardClicked();

	// 백드롭 클릭 시 메뉴 닫힘
	UFUNCTION()
	void HandleBackdropClicked();

private:
	// 화면 전체를 덮는 투명 백드롭 버튼 (외부 클릭 감지용)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackdropButton;

	// 실제 메뉴 버튼들을 담는 패널 — CanvasPanelSlot 위치 조정으로 커서 위치에 배치
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ContentPanel;

	// 장착 버튼 (Weapon / Armor / Trinket 에만 표시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> EquipButton;

	// 사용 버튼 (Consumable 에만 표시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> UseButton;

	// 버리기 버튼 (모든 타입에 표시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DiscardButton;

	// 현재 대상 아이템 인스턴스 ID
	FGuid InstanceID;

	// 주입된 인벤토리 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPBInventoryComponent> InventoryComponent;

	// 주입된 장비 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UPBEquipmentComponent> EquipmentComponent;

	// 주입된 대상 액터 (UseConsumable 이벤트 전송 대상)
	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetActor;
};
