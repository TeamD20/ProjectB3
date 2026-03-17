// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PBFloatingTextWidget.generated.h"

class UTextBlock;
class UPBFloatingTextPayload;
class UWidgetAnimation;

/** 애니메이션 종료 시 호출되는 델리게이트 */
DECLARE_DELEGATE(FOnFloatingTextAnimationFinished);

/**
 * 플로팅 텍스트 UMG 위젯.
 * 수치(데미지/힐)와 라벨 텍스트(Miss/Save 등)를 표시하고,
 * 위젯 애니메이션 종료 시 소유 액터에 알린다.
 * BP 서브클래스에서 애니메이션을 정의한다.
 */
UCLASS(Abstract, BlueprintType)
class PROJECTB3_API UPBFloatingTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 페이로드 기반 텍스트/색상/가시성 설정
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void SetPayload(UPBFloatingTextPayload* Payload);

	// 팝업 + 페이드아웃 애니메이션 재생
	UFUNCTION(BlueprintCallable, Category = "FloatingText")
	void PlayShowAnimation();

public:
	// 애니메이션 종료 시 호출
	FOnFloatingTextAnimationFinished OnFloatingTextAnimationFinished;

protected:
	/*~ UUserWidget Interface ~*/
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

protected:
	// 수치 표시 텍스트블록 ("-12", "+8" 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MagnitudeText;

	// 라벨 텍스트블록 ("Miss", "Save Success" 등)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

	// 일반 팝업 + 페이드아웃 애니메이션 (BP에서 설정)
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> DefaultAnim;

	// 크리티컬 강조 애니메이션 (BP에서 설정)
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> CriticalAnim;

private:
	// 현재 재생 중인 애니메이션
	UPROPERTY()
	TObjectPtr<const UWidgetAnimation> CurrentAnimation;

	// 크리티컬 여부 (SetPayload에서 설정, PlayShowAnimation에서 참조)
	bool bIsCritical = false;
};
