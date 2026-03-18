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
    /** 화자 정보 변경 처리 — 이름, 색상 갱신 */
    void HandleSpeakerChanged(const FPBDialogueParticipantDisplayInfo& InInfo);

    /** 대사 텍스트 변경 처리 — 선택지/주사위 숨기고 대사 패널 표시 */
    void HandleTextChanged(const FText& InText);

    /** 선택지 변경 처리 — 대사/주사위 숨기고 선택지 버튼 생성 */
    void HandleChoicesChanged(const TArray<FPBDialogueChoiceInfo>& InChoices);

    /** 주사위 굴리기 정보 변경 처리 — 대사/선택지 숨기고 주사위 패널 표시 */
    void HandleDiceRollChanged(const FPBDiceRollDisplayInfo& InInfo);

    /** 주사위 결과 변경 처리 — 결과 표시 */
    void HandleDiceResultChanged(const FPBDiceRollDisplayInfo& InResult);

protected:
    // 화자 이름 텍스트 블록
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SpeakerNameText;

    // 대사 텍스트 블록 (Rich Text 지원)
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URichTextBlock> DialogueTextBlock;

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
