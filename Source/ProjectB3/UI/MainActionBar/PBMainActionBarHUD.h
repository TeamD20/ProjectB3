// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBMainActionBarHUD.generated.h"

class UPBSkillBarWidget;
class UPBEquipmentSlotWidget;
class UPBResponseSkillWidget;
class UPBProfileWidget;

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

	// -- [빨강 영역] 스킬바 --
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBSkillBarWidget> SkillBar;

	// -- [노랑 영역] 장비/아이템 슬롯 --
	// 좌측 주무기 슬롯
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBEquipmentSlotWidget> MainWeaponSlots;

	// 우측 유틸리티 슬롯
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBEquipmentSlotWidget> UtilitySlots;

	// -- [초록 영역] 대응 스킬 --
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBResponseSkillWidget> ResponseSkillArea;

	// -- [보라 영역] 정적/프로필 영역 --
	// 프로필 (인벤토리 버튼 포함)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPBProfileWidget> ProfileArea;

	// 시스템 버튼 (+/-) 및 기타 정적 요소는 HUD 블루프린트에서 직접 관리하거나 추가 변수 선언 가능
};
