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

/**
 * 메인 액션바 HUD 최상위 위젯
 * 색상별로 구분된 세부 위젯들을 조립하고 관리합니다.
 */
UCLASS()
class PROJECTB3_API UPBMainActionBarHUD : public UPBWidgetBase
{
	GENERATED_BODY()

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 모든 장비/유틸리티 슬롯을 뷰모델 데이터에 맞춰 다시 생성한다.
	void RefreshEquipmentSlots();

	// 뷰모델 슬롯 갱신 이벤트 핸들러
	void HandleSlotsChanged();

	// 파티원 변경 시 프로필 뷰모델 갱신
	void HandleSelectedPartyMemberChanged(AActor* NewActor);

	// 인벤토리 버튼 클릭 처리
	UFUNCTION()
	void OnInventoryButtonClicked();

	// -- [빨강 영역] 스킬바 --
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBSkillBarWidget> SkillBar;

	// -- [노랑 영역] 장비/아이템 슬롯 --
	// 좌측 주무기 슬롯 (최대 2개)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> MainWeaponSlot1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> MainWeaponSlot2;

	// 우측 유틸리티 슬롯 (최대 4개)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlot1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlot2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlot3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlot4;

	// -- 대응 스킬 --
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBResponseSkillWidget> ResponseSkillArea;
	
	// 프로필
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBProfileWidget> ProfileArea;

	// 우측 인벤토리 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> InventoryButton;

	// 턴 종료 버튼
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> TurnEndButton;

	// 이동 거리 표시 프로그레스 바
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> MovementDistanceProgressBar;

private:
	// 스킬바/장비 ViewModel 참조
	UPROPERTY(Transient)
	TObjectPtr<UPBSkillBarViewModel> SkillBarViewModel;

	// 플레이어 상태 참조
	UPROPERTY(Transient)
	TObjectPtr<APBGameplayPlayerState> CachedPlayerState;

	// SlotsChanged 델리게이트 핸들
	FDelegateHandle SlotsChangedHandle;

	// 파티원 변경 델리게이트 핸들
	FDelegateHandle SelectedPartyMemberChangedHandle;
};
