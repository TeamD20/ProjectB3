// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PBResponseSkillWidget.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBMainActionBarHUD.generated.h"

class UPBSkillBarWidget;
class UPBEquipmentSlotWidget;
class UPBProfileWidget;
class UPBSkillBarViewModel;
class APBGameplayPlayerState;
class UButton;
class UProgressBar;
class UPBCombatManagerSubsystem;
class UPBCombatStatsViewModel;
enum class EPBCombatState : uint8;

/**
 * 메인 액션바 HUD 최상위 위젯
 * 스킬바, 장비 슬롯, 프로필 등 세부 위젯들을 조립하고 관리한다.
 */
UCLASS()
class PROJECTB3_API UPBMainActionBarHUD : public UPBWidgetBase
{
	GENERATED_BODY()

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	// 모든 장비/유틸리티 슬롯을 뷰모델 데이터에 맞춰 갱신
	void RefreshEquipmentSlots();

	// 뷰모델 슬롯 변경 델리게이트 핸들러
	void HandleSlotsChanged();

	// 파티원 변경 시 프로필 뷰모델 갱신
	void HandleSelectedPartyMemberChanged(AActor* NewActor);

	// 인벤토리 버튼 클릭 처리
	UFUNCTION()
	void OnInventoryButtonClicked();

	// 턴 종료 버튼 클릭 처리
	UFUNCTION()
	void OnTurnEndButtonClicked();

	// 활성 턴 변경 델리게이트 핸들러
	void HandleActiveTurnChanged(AActor* Combatant, int32 TurnIndex);

	// 전투 상태 변경 델리게이트 핸들러
	void HandleCombatStateChanged(EPBCombatState NewState);

	// 현재 턴의 진영에 따라 턴 종료 버튼 활성/비활성 갱신
	void UpdateTurnEndButtonState();

	/*~ 이동력 ProgressBar ~*/

	// CombatStatsVM 바인딩 갱신 (파티원 변경 시 호출)
	void BindCombatStatsViewModel(AActor* NewActor);

	// 이동력 비율 변경 핸들러
	void HandleMovementPercentChanged(float NewPercent);

	// BP에서 이동력 프로그래스바 보간 연출 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "MainActionBar")
	void BP_OnMovementPercentChanged(float NewPercent);

protected:
	// 스킬바
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBSkillBarWidget> SkillBar;

	// 주무기 슬롯 1
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> MainWeaponSlot1;

	// 주무기 슬롯 2
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> MainWeaponSlot2;

	// 유틸리티 슬롯 1
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlot1;

	// 유틸리티 슬롯 2
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlot2;

	// 유틸리티 슬롯 3
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlot3;

	// 유틸리티 슬롯 4
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlot4;

	// 대응 스킬 영역
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBResponseSkillWidget> ResponseSkillArea;

	// 프로필 영역
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBProfileWidget> ProfileArea;

	// 인벤토리 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> InventoryButton;

	// 턴 종료 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TurnEndButton;

	// 이동 거리 프로그레스 바
	UPROPERTY(BlueprintReadOnly, Category = "MainActionBar", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> MovementDistanceProgressBar;

private:
	// 스킬바 뷰모델
	UPROPERTY(Transient)
	TObjectPtr<UPBSkillBarViewModel> SkillBarViewModel;

	// 플레이어 상태 캐시
	UPROPERTY(Transient)
	TObjectPtr<APBGameplayPlayerState> CachedPlayerState;

	// 슬롯 변경 델리게이트 핸들
	FDelegateHandle SlotsChangedHandle;

	// 파티원 변경 델리게이트 핸들
	FDelegateHandle SelectedPartyMemberChangedHandle;

	// 활성 턴 변경 델리게이트 핸들
	FDelegateHandle ActiveTurnChangedHandle;

	// 전투 상태 변경 델리게이트 핸들
	FDelegateHandle CombatStateChangedHandle;

	// CombatStatsVM 캐시
	UPROPERTY(Transient)
	TObjectPtr<UPBCombatStatsViewModel> CachedCombatStatsVM;

	// 이동력 비율 변경 델리게이트 핸들
	FDelegateHandle MovementPercentChangedHandle;
};
