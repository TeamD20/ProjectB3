// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBInventoryPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ProjectB3/Player/PBGameplayPlayerState.h"
#include "ProjectB3/UI/Common/PBCombatStatsViewModel.h"
#include "ProjectB3/UI/Inventory/PBEquipSlotWidget.h"
#include "ProjectB3/UI/Inventory/PBInventorySlotWidget.h"
#include "ProjectB3/UI/Inventory/PBInventoryViewModel.h"
#include "ProjectB3/UI/PBUIBlueprintLibrary.h"

void UPBInventoryPanelWidget::InitializeWithViewModel(UPBInventoryViewModel* InViewModel)
{
	// 패널 재초기화 시 이전 바인딩이 남지 않도록 먼저 전체 해제
	UnbindAll();

	InventoryViewModel = InViewModel;
	if (!IsValid(InventoryViewModel))
	{
		return;
	}

	BindInventoryViewModel();
	BindCombatStatsViewModel();
	BindPlayerState();

	CollectEquipSlotWidgets();
	RebuildInventoryGridWidgets();

	HandleCharacterNameChanged(InventoryViewModel->GetCharacterName());
	RefreshAllInventorySlots();
	RefreshAllEquipmentSlots();
	RefreshCombatStats();
	RefreshGoldText();
	RefreshWeaponSetHighlight();

	// 패널이 표시될 때 프리뷰 캡처를 활성화하고 렌더 타겟을 Image에 적용
	InventoryViewModel->SetPreviewCaptureActive(true);
	ApplyCharacterRenderTarget();
}

void UPBInventoryPanelWidget::NativeDestruct()
{
	// 패널이 소멸할 때 프리뷰 캡처를 비활성화해 불필요한 드로우콜을 방지
	if (IsValid(InventoryViewModel))
	{
		InventoryViewModel->SetPreviewCaptureActive(false);
	}

	UnbindAll();
	Super::NativeDestruct();
}

void UPBInventoryPanelWidget::BindInventoryViewModel()
{
	if (!IsValid(InventoryViewModel))
	{
		return;
	}

	// 인벤토리/장비는 개별 갱신 + 전체 갱신 2계층 이벤트를 모두 구독
	InventorySlotUpdatedHandle = InventoryViewModel->OnInventorySlotUpdated.AddUObject(this, &ThisClass::HandleInventorySlotUpdated);
	InventoryFullRefreshHandle = InventoryViewModel->OnInventoryFullRefresh.AddUObject(this, &ThisClass::HandleInventoryFullRefresh);
	EquipmentSlotUpdatedHandle = InventoryViewModel->OnEquipmentSlotUpdated.AddUObject(this, &ThisClass::HandleEquipmentSlotUpdated);
	WeaponSetChangedHandle = InventoryViewModel->OnWeaponSetChanged.AddUObject(this, &ThisClass::HandleWeaponSetChanged);
	CharacterNameChangedHandle = InventoryViewModel->OnCharacterNameChanged.AddUObject(this, &ThisClass::HandleCharacterNameChanged);
}

void UPBInventoryPanelWidget::BindCombatStatsViewModel()
{
	if (!IsValid(InventoryViewModel))
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	AActor* TargetActor = InventoryViewModel->GetTargetActor();
	if (!IsValid(LocalPlayer) || !IsValid(TargetActor))
	{
		return;
	}

	// 동일 Actor의 CombatStats VM을 조합해 AC/명중/DC를 함께 표시
	CombatStatsViewModel = UPBUIBlueprintLibrary::GetOrCreateActorViewModel<UPBCombatStatsViewModel>(LocalPlayer, TargetActor);
	if (!IsValid(CombatStatsViewModel))
	{
		return;
	}

	ArmorClassChangedHandle = CombatStatsViewModel->OnArmorClassChanged.AddUObject(this, &ThisClass::HandleCombatStatsChanged);
	HitBonusChangedHandle = CombatStatsViewModel->OnHitBonusChanged.AddUObject(this, &ThisClass::HandleCombatStatsChanged);
	SpellSaveDCChangedHandle = CombatStatsViewModel->OnSpellSaveDCChanged.AddUObject(this, &ThisClass::HandleCombatStatsChanged);
}

void UPBInventoryPanelWidget::BindPlayerState()
{
	// 골드는 파티 공유 재화이므로 PlayerState 단일 소스를 구독
	APlayerController* OwningPlayer = GetOwningPlayer();
	CachedPlayerState = IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<APBGameplayPlayerState>() : nullptr;

	if (IsValid(CachedPlayerState))
	{
		CachedPlayerState->OnGoldChanged.AddDynamic(this, &ThisClass::HandleGoldChanged);
	}
}

void UPBInventoryPanelWidget::UnbindAll()
{
	// 뷰모델/플레이어스테이트 이벤트를 모두 해제해 중복 콜백을 방지
	if (IsValid(InventoryViewModel))
	{
		if (InventorySlotUpdatedHandle.IsValid())
		{
			InventoryViewModel->OnInventorySlotUpdated.Remove(InventorySlotUpdatedHandle);
			InventorySlotUpdatedHandle.Reset();
		}

		if (InventoryFullRefreshHandle.IsValid())
		{
			InventoryViewModel->OnInventoryFullRefresh.Remove(InventoryFullRefreshHandle);
			InventoryFullRefreshHandle.Reset();
		}

		if (EquipmentSlotUpdatedHandle.IsValid())
		{
			InventoryViewModel->OnEquipmentSlotUpdated.Remove(EquipmentSlotUpdatedHandle);
			EquipmentSlotUpdatedHandle.Reset();
		}

		if (WeaponSetChangedHandle.IsValid())
		{
			InventoryViewModel->OnWeaponSetChanged.Remove(WeaponSetChangedHandle);
			WeaponSetChangedHandle.Reset();
		}

		if (CharacterNameChangedHandle.IsValid())
		{
			InventoryViewModel->OnCharacterNameChanged.Remove(CharacterNameChangedHandle);
			CharacterNameChangedHandle.Reset();
		}
	}

	if (IsValid(CombatStatsViewModel))
	{
		if (ArmorClassChangedHandle.IsValid())
		{
			CombatStatsViewModel->OnArmorClassChanged.Remove(ArmorClassChangedHandle);
			ArmorClassChangedHandle.Reset();
		}

		if (HitBonusChangedHandle.IsValid())
		{
			CombatStatsViewModel->OnHitBonusChanged.Remove(HitBonusChangedHandle);
			HitBonusChangedHandle.Reset();
		}

		if (SpellSaveDCChangedHandle.IsValid())
		{
			CombatStatsViewModel->OnSpellSaveDCChanged.Remove(SpellSaveDCChangedHandle);
			SpellSaveDCChangedHandle.Reset();
		}
	}

	if (IsValid(CachedPlayerState))
	{
		CachedPlayerState->OnGoldChanged.RemoveDynamic(this, &ThisClass::HandleGoldChanged);
	}

	// 내부 참조 캐시를 명시적으로 초기화
	CachedPlayerState = nullptr;
	CombatStatsViewModel = nullptr;
	InventoryViewModel = nullptr;
	InventorySlotWidgets.Reset();
	EquipSlotWidgets.Reset();
}

void UPBInventoryPanelWidget::ApplyCharacterRenderTarget()
{
	if (!IsValid(CharacterRenderImage) || !IsValid(InventoryViewModel))
	{
		return;
	}

	UTextureRenderTarget2D* RenderTarget = InventoryViewModel->GetCharacterRenderTarget();
	if (!IsValid(RenderTarget))
	{
		return;
	}

	// RenderTarget을 FSlateBrush로 감싸 Image 위젯에 설정
	FSlateBrush Brush;
	Brush.SetResourceObject(RenderTarget);
	CharacterRenderImage->SetBrush(Brush);
}

void UPBInventoryPanelWidget::CollectEquipSlotWidgets()
{
	EquipSlotWidgets.Reset();

	if (!IsValid(WidgetTree) || !IsValid(InventoryViewModel))
	{
		return;
	}

	TArray<UWidget*> AllWidgets;
	WidgetTree->GetAllWidgets(AllWidgets);

	for (UWidget* Widget : AllWidgets)
	{
		UPBEquipSlotWidget* EquipSlotWidget = Cast<UPBEquipSlotWidget>(Widget);
		if (!IsValid(EquipSlotWidget))
		{
			continue;
		}

		EquipSlotWidget->InitializeSlot(InventoryViewModel);
		// 동일 BoundSlot이 중복 배치되면 마지막 위젯 기준으로 덮어쓴다.
		EquipSlotWidgets.Add(EquipSlotWidget->GetBoundSlot(), EquipSlotWidget);
	}
}

void UPBInventoryPanelWidget::RebuildInventoryGridWidgets()
{
	InventorySlotWidgets.Reset();

	if (!IsValid(InventoryGrid) || !IsValid(InventorySlotWidgetClass) || !IsValid(InventoryViewModel))
	{
		return;
	}

	InventoryGrid->ClearChildren();

	// 에디터 설정 실수(0 이하 열 수)로 인한 나눗셈 예외를 방지
	const int32 SafeColumns = FMath::Max(1, InventoryGridColumns);

	const int32 SlotCount = InventoryViewModel->GetInventorySlotCount();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UPBInventorySlotWidget* SlotWidget = CreateWidget<UPBInventorySlotWidget>(this, InventorySlotWidgetClass);
		if (!IsValid(SlotWidget))
		{
			continue;
		}

		SlotWidget->InitializeSlot(SlotIndex, InventoryViewModel);
		InventorySlotWidgets.Add(SlotWidget);

		if (UUniformGridPanel* UniformGrid = Cast<UUniformGridPanel>(InventoryGrid))
		{
			const int32 GridRow = SlotIndex / SafeColumns;
			const int32 GridColumn = SlotIndex % SafeColumns;
			UniformGrid->AddChildToUniformGrid(SlotWidget, GridRow, GridColumn);
		}
		else
		{
			InventoryGrid->AddChild(SlotWidget);
		}
	}
}

void UPBInventoryPanelWidget::RefreshAllInventorySlots()
{
	for (UPBInventorySlotWidget* SlotWidget : InventorySlotWidgets)
	{
		if (IsValid(SlotWidget))
		{
			SlotWidget->RefreshSlot();
		}
	}
}

void UPBInventoryPanelWidget::RefreshAllEquipmentSlots()
{
	for (const TPair<EPBEquipSlot, TObjectPtr<UPBEquipSlotWidget>>& Pair : EquipSlotWidgets)
	{
		if (IsValid(Pair.Value))
		{
			Pair.Value->RefreshSlot();
		}
	}
}

void UPBInventoryPanelWidget::RefreshCombatStats()
{
	if (IsValid(ArmorClassText))
	{
		const int32 AC = IsValid(CombatStatsViewModel) ? CombatStatsViewModel->GetArmorClass() : 0;
		ArmorClassText->SetText(FText::AsNumber(AC));
	}

	if (IsValid(HitBonusText))
	{
		const int32 HitBonus = IsValid(CombatStatsViewModel) ? CombatStatsViewModel->GetHitBonus() : 0;
		HitBonusText->SetText(FText::AsNumber(HitBonus));
	}

	if (IsValid(SpellSaveDCText))
	{
		const int32 SpellDC = IsValid(CombatStatsViewModel) ? CombatStatsViewModel->GetSpellSaveDC() : 0;
		SpellSaveDCText->SetText(FText::AsNumber(SpellDC));
	}
}

void UPBInventoryPanelWidget::RefreshGoldText()
{
	if (!IsValid(GoldText))
	{
		return;
	}

	const int32 CurrentGold = IsValid(CachedPlayerState) ? CachedPlayerState->GetGold() : 0;
	GoldText->SetText(FText::AsNumber(CurrentGold));
}

void UPBInventoryPanelWidget::RefreshWeaponSetHighlight()
{
	if (!IsValid(InventoryViewModel))
	{
		return;
	}

	const int32 ActiveSet = InventoryViewModel->GetActiveWeaponSet();

	if (UPBEquipSlotWidget* MainSet1 = EquipSlotWidgets.FindRef(EPBEquipSlot::WeaponSet1_Main))
	{
		MainSet1->SetHighlighted(ActiveSet == 1);
	}
	if (UPBEquipSlotWidget* OffSet1 = EquipSlotWidgets.FindRef(EPBEquipSlot::WeaponSet1_Off))
	{
		OffSet1->SetHighlighted(ActiveSet == 1);
	}
	if (UPBEquipSlotWidget* MainSet2 = EquipSlotWidgets.FindRef(EPBEquipSlot::WeaponSet2_Main))
	{
		MainSet2->SetHighlighted(ActiveSet == 2);
	}
	if (UPBEquipSlotWidget* OffSet2 = EquipSlotWidgets.FindRef(EPBEquipSlot::WeaponSet2_Off))
	{
		OffSet2->SetHighlighted(ActiveSet == 2);
	}
}

void UPBInventoryPanelWidget::HandleInventorySlotUpdated(int32 SlotIndex)
{
	if (InventorySlotWidgets.IsValidIndex(SlotIndex) && IsValid(InventorySlotWidgets[SlotIndex]))
	{
		InventorySlotWidgets[SlotIndex]->RefreshSlot();
	}
}

void UPBInventoryPanelWidget::HandleInventoryFullRefresh()
{
	const int32 RequiredSlots = IsValid(InventoryViewModel) ? InventoryViewModel->GetInventorySlotCount() : 0;
	if (InventorySlotWidgets.Num() != RequiredSlots)
	{
		RebuildInventoryGridWidgets();
	}

	RefreshAllInventorySlots();
}

void UPBInventoryPanelWidget::HandleEquipmentSlotUpdated(EPBEquipSlot UpdatedSlot)
{
	if (UPBEquipSlotWidget* EquipWidget = EquipSlotWidgets.FindRef(UpdatedSlot))
	{
		EquipWidget->RefreshSlot();
	}
}

void UPBInventoryPanelWidget::HandleWeaponSetChanged(int32 NewWeaponSet)
{
	(void)NewWeaponSet;
	RefreshWeaponSetHighlight();
}

void UPBInventoryPanelWidget::HandleCharacterNameChanged(FText NewCharacterName)
{
	if (IsValid(CharacterNameText))
	{
		CharacterNameText->SetText(NewCharacterName);
	}
}

void UPBInventoryPanelWidget::HandleCombatStatsChanged(int32 NewValue)
{
	(void)NewValue;
	RefreshCombatStats();
}

void UPBInventoryPanelWidget::HandleGoldChanged(int32 NewGold)
{
	if (IsValid(GoldText))
	{
		GoldText->SetText(FText::AsNumber(NewGold));
	}
}
