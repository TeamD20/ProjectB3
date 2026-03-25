// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/ItemSystem/PBItemTypes.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBInventoryPanelWidget.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

class APBGameplayPlayerState;
class UImage;
class UTextBlock;
class UPanelWidget;
class UPBCombatStatsViewModel;
class UPBEquipSlotWidget;
class UPBInventoryContextMenuWidget;
class UPBInventorySlotWidget;
class UPBInventoryViewModel;

// 개별 파티원 인벤토리 패널 위젯
UCLASS()
class PROJECTB3_API UPBInventoryPanelWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 파티원 Actor-Bound InventoryViewModel을 바인딩
	void InitializeWithViewModel(UPBInventoryViewModel* InViewModel);

protected:
	/*~ UUserWidget Interface ~*/
	// 위젯 소멸 시 모든 델리게이트를 정리
	virtual void NativeDestruct() override;

private:
	// 인벤토리 ViewModel 이벤트를 바인딩
	void BindInventoryViewModel();

	// 전투 스탯 ViewModel을 획득하고 이벤트를 바인딩
	void BindCombatStatsViewModel();

	// PlayerState 골드 이벤트를 바인딩
	void BindPlayerState();

	// 모든 바인딩을 해제
	void UnbindAll();

	// 캐릭터 렌더 타겟을 CharacterRenderImage에 적용
	void ApplyCharacterRenderTarget();

	// 장비 슬롯 위젯을 수집하고 초기화
	void CollectEquipSlotWidgets();

	// 인벤토리 그리드 슬롯 위젯을 재생성
	void RebuildInventoryGridWidgets();

	// 인벤토리 슬롯 위젯을 전체 갱신
	void RefreshAllInventorySlots();

	// 장비 슬롯 위젯을 전체 갱신
	void RefreshAllEquipmentSlots();

	// 전투 스탯 텍스트를 갱신
	void RefreshCombatStats();

	// 골드 텍스트를 갱신
	void RefreshGoldText();

	// 활성 무기 세트 하이라이트를 갱신
	void RefreshWeaponSetHighlight();

	// 인벤토리 단일 슬롯 변경 이벤트를 처리
	void HandleInventorySlotUpdated(int32 SlotIndex);

	// 인벤토리 전체 갱신 이벤트를 처리
	void HandleInventoryFullRefresh();

	// 장비 단일 슬롯 변경 이벤트를 처리
	void HandleEquipmentSlotUpdated(EPBEquipSlot UpdatedSlot);

	// 무기 세트 변경 이벤트를 처리
	void HandleWeaponSetChanged(int32 NewWeaponSet);

	// 캐릭터 이름 변경 이벤트를 처리
	void HandleCharacterNameChanged(FText NewCharacterName);

	// 전투 스탯 변경 이벤트를 처리
	void HandleCombatStatsChanged(int32 NewValue);

	// 골드 변경 이벤트를 처리
	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);

	// 슬롯 우클릭 컨텍스트 메뉴 표시 요청을 처리
	void HandleSlotContextMenuRequested(
		const FGuid& InstanceID,
		EPBItemType ItemType,
		FVector2D ScreenPosition);

protected:
	// 캐릭터 3D 프리뷰 이미지 (RenderTarget 표시)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> CharacterRenderImage;

	// 캐릭터 이름 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CharacterNameText;

	// 방어력 표시 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ArmorClassText;

	// 명중 보너스 표시 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HitBonusText;

	// 주문 난이도 표시 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpellSaveDCText;

	// 골드 표시 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	// 인벤토리 슬롯 그리드 패널
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> InventoryGrid;

	// 인벤토리 슬롯 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPBInventorySlotWidget> InventorySlotWidgetClass;

	// 컨텍스트 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPBInventoryContextMenuWidget> ContextMenuWidgetClass;

	// 인벤토리 그리드 열 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 InventoryGridColumns = 5;

	// 캐릭터 배경 렌더링 머티리얼 (M_Character_BG_Render 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Character", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> CharacterBGMaterial;

private:
	// 현재 바인딩된 인벤토리 ViewModel
	UPROPERTY(Transient)
	TObjectPtr<UPBInventoryViewModel> InventoryViewModel;

	// 현재 바인딩된 전투 스탯 ViewModel
	UPROPERTY(Transient)
	TObjectPtr<UPBCombatStatsViewModel> CombatStatsViewModel;

	// 현재 바인딩된 PlayerState
	UPROPERTY(Transient)
	TObjectPtr<APBGameplayPlayerState> CachedPlayerState;

	// 인벤토리 슬롯 위젯 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPBInventorySlotWidget>> InventorySlotWidgets;

	// 장비 슬롯 위젯 캐시
	UPROPERTY(Transient)
	TMap<EPBEquipSlot, TObjectPtr<UPBEquipSlotWidget>> EquipSlotWidgets;

	// 컨텍스트 메뉴 위젯 캐시 (패널 당 1개 재사용)
	UPROPERTY(Transient)
	TObjectPtr<UPBInventoryContextMenuWidget> CachedContextMenuWidget;

	// 캐릭터 배경 DMI 캐시 (패널 재초기화 시 재생성)
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CachedCharacterDMI;

	// Inventory 슬롯 갱신 델리게이트 핸들
	FDelegateHandle InventorySlotUpdatedHandle;

	// Inventory 전체 갱신 델리게이트 핸들
	FDelegateHandle InventoryFullRefreshHandle;

	// Equipment 슬롯 갱신 델리게이트 핸들
	FDelegateHandle EquipmentSlotUpdatedHandle;

	// 무기 세트 변경 델리게이트 핸들
	FDelegateHandle WeaponSetChangedHandle;

	// 캐릭터 이름 변경 델리게이트 핸들
	FDelegateHandle CharacterNameChangedHandle;

	// AC 변경 델리게이트 핸들
	FDelegateHandle ArmorClassChangedHandle;

	// 명중 보너스 변경 델리게이트 핸들
	FDelegateHandle HitBonusChangedHandle;

	// 주문 난이도 변경 델리게이트 핸들
	FDelegateHandle SpellSaveDCChangedHandle;
};
