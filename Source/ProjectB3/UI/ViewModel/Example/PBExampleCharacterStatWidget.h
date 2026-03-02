// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBExampleCharacterStatWidget.generated.h"

class SProgressBar;
class UPBExampleCharacterStatViewModel;

/**
 * CharacterStatViewModel을 사용하는 Slate 기반 위젯 예시.
 * Actor-Bound ViewModel과 바인딩하여 D&D 능력치를 표시한다.
 *
 * 외부에서 SetTargetActor()를 호출하면 해당 Actor의 ViewModel에 바인딩된다.
 */
UCLASS(meta = (DisplayName = "Character Stat Widget (Example)"))
class PROJECTB3_API UPBExampleCharacterStatWidget : public UPBWidgetBase
{
	GENERATED_BODY()

public:
	UPBExampleCharacterStatWidget(const FObjectInitializer& ObjectInitializer);

	// 표시할 Actor 설정 (Actor-Bound ViewModel 바인딩)
	UFUNCTION(BlueprintCallable, Category = "Character Stat")
	void SetTargetActor(AActor* InActor);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// ViewModel 바인딩/해제
	void BindToViewModel(UPBExampleCharacterStatViewModel* InVM);
	void UnbindFromViewModel();

	// ViewModel 델리게이트 콜백
	UFUNCTION()
	void HandleStatChanged();

	UFUNCTION()
	void HandleHPChanged(int32 NewHP, int32 NewMaxHP);

	// Slate UI 갱신
	void RefreshAllStats();
	void RefreshHP();

	// 능력치 행 생성 헬퍼
	TSharedRef<SWidget> MakeAbilityRow(const FString& Label, int32 Score, int32 Modifier);

	// ViewModel 캐시
	UPROPERTY()
	TObjectPtr<UPBExampleCharacterStatViewModel> CachedViewModel;

	// Slate 위젯 참조
	TSharedPtr<SVerticalBox> RootBox;
	TSharedPtr<STextBlock> NameText;
	TSharedPtr<STextBlock> LevelText;
	TSharedPtr<STextBlock> HPText;
	TSharedPtr<SProgressBar> HPBar;
	TSharedPtr<STextBlock> ACText;
	TSharedPtr<SVerticalBox> AbilityScoresBox;
};
