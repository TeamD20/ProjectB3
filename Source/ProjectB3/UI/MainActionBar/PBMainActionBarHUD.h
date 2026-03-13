// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBMainActionBarHUD.generated.h"

class UPBSkillBarWidget;
class UPBEquipmentSlotWidget;
class UPBResponseSkillWidget;
class UPBProfileWidget;
class UPBSkillBarViewModel;
class UPanelWidget;

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

	// -- [빨강 영역] 스킬바 --
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBSkillBarWidget> SkillBar;

	// -- [노랑 영역] 장비/아이템 슬롯 --
	// 좌측 주무기 슬롯들을 담을 컨테이너 (WrapBox/HorizontalBox 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> MainWeaponSlots;

	// 우측 유틸리티 슬롯들을 담을 컨테이너 (WrapBox/HorizontalBox 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> UtilitySlots;

	// 개별 장비/아이템 슬롯 위젯 클래스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|EquipmentSlot", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPBEquipmentSlotWidget> EquipmentSlotWidgetClass;

	// -- [초록 영역] 대응 스킬 --
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBResponseSkillWidget> ResponseSkillArea;

	// -- [보라 영역] 정적/프로필 영역 --
	// 프로필 (인벤토리 버튼 포함)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBProfileWidget> ProfileArea;

	// 기타 정적 요소는 HUD 블루프린트에서 직접 관리하거나 추가 변수 선언 가능

private:
	// 스킬바/장비 ViewModel 참조
	UPROPERTY(Transient)
	TObjectPtr<UPBSkillBarViewModel> SkillBarViewModel;

	// SlotsChanged 델리게이트 핸들
	FDelegateHandle SlotsChangedHandle;
};
