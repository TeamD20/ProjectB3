// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "PBDialogueWidget.generated.h"

class UOverlay;
class UPBDialogueViewModel;
class UPBDialogueChoiceEntryWidget;
class UTextBlock;
class URichTextBlock;
class UImage;
class UVerticalBox;
class UPBDiceRollSubWidget;
class UWidget;
struct FGeometry;
struct FPointerEvent;
struct FKeyEvent;

/**
 * 대화 메인 위젯.
 * ViewModel의 각 델리게이트에 바인딩하여 화자/대사/선택지/주사위 패널을 전환한다.
 */
UCLASS(Abstract)
class PROJECTB3_API UPBDialogueWidget : public UPBWidgetBase
{
    GENERATED_BODY()

public:
    /*~ UUserWidget Interface ~*/
    /** ViewModel 델리게이트 바인딩 */
    virtual void NativeConstruct() override;

    /** 바인딩 해제 */
    virtual void NativeDestruct() override;

    /** 대사 진행 버튼 클릭 처리 */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void RequestContinueByButtonClick();

protected:
    /*~ UUserWidget Interface ~*/
    /** 대사 표시 중 마우스 좌클릭으로 진행 */
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    /** 대사 표시 중 키 입력으로 진행 */
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

protected:
    /** 대사 텍스트 변경 처리 — [화자이름] 텍스트 포맷으로 표시, 선택지/주사위 숨김 */
    void HandleTextChanged(const FText& InText);

    /** 선택지 변경 처리 — 대사/주사위 숨기고 선택지 버튼 생성. 프롬프트 텍스트가 있으면 상단에 표시 */
    void HandleChoicesChanged(const TArray<FPBDialogueChoiceInfo>& InChoices, const FText& InPromptText);

    /** 주사위 굴리기 정보 변경 처리 — 대사/선택지 숨기고 주사위 패널 표시 */
    void HandleDiceRollChanged(const FPBDiceRollDisplayInfo& InInfo);

    /** 주사위 결과 변경 처리 — 결과 표시 */
    void HandleDiceResultChanged(const FPBDiceRollDisplayInfo& InResult);

protected:
    // 대사 텍스트 블록 (Rich Text 지원, "[화자이름] 대사" 형태로 표시)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URichTextBlock> DialogueTextBlock;

    // 선택지 표시 시 상단에 노출할 이전 노드 대사 텍스트 블록
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URichTextBlock> ChoicePromptTextBlock;

    // 선택지 컨테이너
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UOverlay> ChoiceOverlay;
    
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> ChoicePanel;

    // 주사위 굴리기 서브위젯
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPBDiceRollSubWidget> DiceRollPanel;

    // "계속하려면 클릭" 안내 위젯
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> ContinuePrompt;

    // 선택지 항목 위젯 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
    TSubclassOf<UPBDialogueChoiceEntryWidget> ChoiceEntryWidgetClass;

private:
    /** 현재 상태에서 대사 진행 입력 허용 여부 확인 */
    bool CanRequestContinueFromInput() const;

    /** 대사 진행 요청 공통 처리 */
    void RequestContinueInternal();

    /** 대사 진행으로 인식할 키인지 확인 */
    bool IsContinueKey(const FKey& InKey) const;

private:
    // 바인딩된 ViewModel (약참조)
    TWeakObjectPtr<UPBDialogueViewModel> BoundViewModel;
};
