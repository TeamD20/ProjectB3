// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBActionIndicatorTypes.h"
#include "PBActionIndicatorHUD.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UPBActionIndicatorViewModel;
class UWidgetAnimation;

/**
 * 화면 HUD에 캐릭터의 현재 행동 상태(스킬 시전, 대기, 반응, 이동 중)를 표시하는 위젯.
 */
UCLASS(Abstract)
class PROJECTB3_API UPBActionIndicatorHUD : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	// 외부(보통 MainActionBarHUD나 최상위 UI)에서 ViewModel 등록 시 호출
	UFUNCTION(BlueprintCallable, Category = "ActionIndicator")
	void SetupViewModel(UPBActionIndicatorViewModel* InViewModel);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ViewModel 이벤트 구독 핸들러
	UFUNCTION()
	void HandleActionChanged(const FPBActionIndicatorData& Data);

	// 등장 애니메이션 실행 (ViewModel에서 bIsActive == true 시 호출)
	UFUNCTION(BlueprintCallable, Category = "ActionIndicator")
	void PlayStartAnim();

	// 퇴장 애니메이션 실행 (ViewModel에서 bIsActive == false 시 호출)
	UFUNCTION(BlueprintCallable, Category = "ActionIndicator")
	void PlayEndAnim();

	// BP에서 연출 커스텀 (예: 텍스트 변경 애니메이션, 아이콘 교체 등)
	UFUNCTION(BlueprintImplementableEvent, Category = "ActionIndicator")
	void BP_OnActionChanged(const FPBActionIndicatorData& Data);

protected:
	// 바인딩된 뷰모델
	UPROPERTY(BlueprintReadOnly, Category = "ActionIndicator")
	UPBActionIndicatorViewModel* ActionViewModel;

	// 인디케이터 컨테이너 테두리 (색상/투명도 조절용)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> IndicatorBorder;

	// 행동 설명 텍스트 (예: "파이어볼 시전 중")
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionText;

	// 행동 아이콘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ActionIcon;

	// 등장 시 재생할 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> StartAnim;

	// 퇴장 시 재생할 애니메이션
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> EndAnim;
};
