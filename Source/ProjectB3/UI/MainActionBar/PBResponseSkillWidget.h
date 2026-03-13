// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBResponseSkillWidget.generated.h"

class UPBSkillBarViewModel;
class UPBSkillSlotWidget;
class UPanelWidget;

/** [초록 영역] 대응 스킬 표시 위젯 */
UCLASS(Abstract)
class PROJECTB3_API UPBResponseSkillWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 뷰모델 주입 및 초기화
	void InitializeResponseArea(UPBSkillBarViewModel* InViewModel);

protected:
	virtual void NativeDestruct() override;

	// 뷰모델 데이터에 맞춰 슬롯을 동적으로 생성/갱신
	void RefreshResponseSkills();

	// 뷰모델 변경 이벤트 핸들러
	void HandleSlotsChanged();

protected:
	// 슬롯이 추가될 컨테이너 (Horizontal Box 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> ResponseSkillContainer;

	// 동적으로 생성할 슬롯 위젯 클래스
	UPROPERTY(EditAnywhere, Category = "UI|Response")
	TSubclassOf<UPBSkillSlotWidget> SkillSlotWidgetClass;

private:
	// 뷰모델 참조
	UPROPERTY(Transient)
	TObjectPtr<UPBSkillBarViewModel> SkillBarViewModel;

	// 이벤트 핸들 보관
	FDelegateHandle SlotsChangedHandle;
};
