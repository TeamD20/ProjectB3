// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectB3/AbilitySystem/PBAbilityTypes.h"
#include "PBSkillNameFloatingWidget.generated.h"

class UTextBlock;
class UImage;
class UWidgetAnimation;

/** 애니메이션 종료 시 호출되는 델리게이트 */
DECLARE_DELEGATE(FOnSkillNameFloatingAnimationFinished);

/**
 * 스킬 이름 플로팅 표시 위젯.
 * 스킬 시전 시 캐릭터 머리 위에 팝업 텍스트 표시
 */
UCLASS(Abstract, BlueprintType)
class PROJECTB3_API UPBSkillNameFloatingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 스킬 이름 및 어빌리티 타입(아이콘 결정 등에 사용 가능) 설정
	UFUNCTION(BlueprintCallable, Category = "UI|Combat")
	void SetSkillInfo(const FText& SkillName, EPBAbilityType AbilityType);

	// 팝업 애니메이션 재생
	UFUNCTION(BlueprintCallable, Category = "UI|Combat")
	void PlayShowAnimation();

public:
	// 애니메이션 종료 시 호출 (소유 액터 파괴용)
	FOnSkillNameFloatingAnimationFinished OnAnimFinished;

protected:
	/*~ UUserWidget Interface ~*/
	virtual void OnAnimationFinished_Implementation(const UWidgetAnimation* Animation) override;

protected:
	// 스킬 이름 표시용 (예: "파이어볼")
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkillNameText;

	// 스킬 자원 유형 아이콘 (선택사항)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> AbilityTypeIcon;

	// 팝업 + 페이드아웃 애니메이션 (BP)
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> PopupAnim;

private:
	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> CurrentAnim;
};
