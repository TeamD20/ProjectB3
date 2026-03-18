// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBCombatLogTypes.h"
#include "PBCombatLogWidget.generated.h"

class UScrollBox;
class UPBCombatLogEntryWidget;
class UPBCombatLogViewModel;

/** 컴뱃 로그 패널 위젯. ViewModel에 바인딩하여 로그 항목을 ScrollBox에 동적으로 추가한다. */
UCLASS(Abstract)
class PROJECTB3_API UPBCombatLogWidget : public UPBWidgetBase
{
	GENERATED_BODY()

protected:
	/*~ UUserWidget Interface ~*/
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// 새 로그 엔트리 위젯을 ScrollBox에 추가한다.
	void AddEntryWidget(const FPBCombatLogEntry& Entry);

	// ScrollBox의 모든 엔트리 위젯을 제거한다.
	void ClearEntryWidgets();

	// 기존 로그 목록 전체를 ScrollBox에 복원한다.
	void RebuildAllEntries();

private:
	// 로그 엔트리 스크롤 컨테이너
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox;

	// 개별 로그 줄 위젯 클래스 (BP에서 지정)
	UPROPERTY(EditDefaultsOnly, Category = "CombatLog")
	TSubclassOf<UPBCombatLogEntryWidget> EntryWidgetClass;

	// 바인딩된 ViewModel
	UPROPERTY()
	TObjectPtr<UPBCombatLogViewModel> CombatLogVM;

	// 사용자가 스크롤 중이면 자동 스크롤 비활성화
	bool bAutoScrollEnabled = true;

	// ViewModel 델리게이트 핸들
	FDelegateHandle EntryAddedHandle;
	FDelegateHandle LogClearedHandle;
};
