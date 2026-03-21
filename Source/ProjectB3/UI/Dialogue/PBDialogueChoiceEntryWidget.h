// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "PBDialogueChoiceEntryWidget.generated.h"

class URichTextBlock;
class UButton;
class UImage;
class UTextBlock;
class UPBDialogueViewModel;
struct FGeometry;
struct FPointerEvent;

/**
 * 대화 선택지 항목 위젯.
 * InitializeChoice()로 선택지 정보와 ViewModel 인덱스를 설정하고,
 * 버튼 클릭 시 ViewModel->SelectChoice(ChoiceIndex)를 호출한다.
 */
UCLASS(Abstract)
class PROJECTB3_API UPBDialogueChoiceEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /*~ UUserWidget Interface ~*/
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    /** 선택지 정보와 선택 인덱스, ViewModel을 바인딩하여 위젯을 초기화 */
    void InitializeChoice(const FPBDialogueChoiceInfo& InInfo, int32 InChoiceIndex, UPBDialogueViewModel* InViewModel);

protected:
    // 선택지 버튼
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> ChoiceButton;

    // 선택지 텍스트
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<URichTextBlock> ChoiceText;

    // 비활성 사유 텍스트 (bAvailable == false 시 표시)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URichTextBlock> UnavailableReasonText;

    // 호버 시 활성화되는 인디케이터 아이콘
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> IndicatorImage;

    // 기본 텍스트 색상
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
    FLinearColor DefaultTextColor = FLinearColor::White;

    // 호버 텍스트 색상
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Appearance")
    FLinearColor HoveredTextColor = FLinearColor::Yellow;

private:
    /** 버튼 클릭 시 ViewModel에 선택 인덱스 전달 */
    UFUNCTION()
    void HandleChoiceButtonClicked();

    /** 선택 입력 공통 처리 */
    void RequestSelectChoice();

private:
    // 이 항목의 선택 인덱스
    int32 ChoiceIndex = 0;

    // 선택 가능 여부
    bool bChoiceAvailable = true;

    // 연결된 ViewModel (약참조)
    TWeakObjectPtr<UPBDialogueViewModel> BoundViewModel;
};
