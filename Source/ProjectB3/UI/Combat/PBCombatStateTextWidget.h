// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PBCombatStateTextWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

/** 애니메이션 종료 시 호출되는 델리게이트 */
DECLARE_DELEGATE(FOnCombatStateTextAnimationFinished);

/**
 * 전투 진입/종료 상태 텍스트 표시 위젯.
 * 전투 시작 시 "전투 시작!", 종료 시 "전투 종료!" 팝업 텍스트를 표시하고 페이드아웃한다.
 */
UCLASS(Abstract, BlueprintType)
class PROJECTB3_API UPBCombatStateTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 상태 텍스트를 설정하고 애니메이션을 재생
	UFUNCTION(BlueprintCallable, Category = "UI|Combat")
	void SetCombatState(bool bIsStarting);

public:
	// 애니메이션 종료 시 호출 (소유 액터가 구독하여 자신을 Destroy)
	FOnCombatStateTextAnimationFinished OnAnimFinished;

protected:
	/*~ UUserWidget Interface ~*/
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

protected:
	// "전투 시작!" 또는 "전투 종료!"를 표시할 텍스트 블록
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StateText;

	// 전투 시작 애니메이션 (BP)
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> EntryAnim;

	// 전투 종료 애니메이션 (BP)
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ExitAnim;

private:
	// 현재 재생 중인 애니메이션
	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> CurrentAnim;
};
