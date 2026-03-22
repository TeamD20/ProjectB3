// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "PBDiceRollSubWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 주사위 굴리기 서브위젯.
 * 스킬명, DC, 수정치를 표시하고 굴리기 버튼을 제공한다.
 * 결과 표시도 담당한다.
 */
UCLASS(Abstract)
class PROJECTB3_API UPBDiceRollSubWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 주사위 굴리기 정보를 설정하고 UI를 초기화 */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void InitializeDiceRoll(const FPBDiceRollDisplayInfo& InInfo);

    /** 주사위 결과를 표시. 성공/실패에 따라 BP 애니메이션 이벤트를 호출한다 */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void ShowDiceResult(const FPBDiceRollDisplayInfo& InResult);

    /** 결과가 표시된 상태인지 반환. DialogueWidget의 진행 입력 허용 여부 판단에 사용 */
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsResultShown() const { return bResultShown; }

protected:
    /*~ UUserWidget Interface ~*/
    virtual void NativeConstruct() override;

    /** 성공 연출 애니메이션. BP에서 UMG Animation으로 구현 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
    void PlaySuccessAnimation(const FPBDiceRollDisplayInfo& InResult);

    /** 실패 연출 애니메이션. BP에서 UMG Animation으로 구현 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
    void PlayFailureAnimation(const FPBDiceRollDisplayInfo& InResult);

private:
    /** 굴리기 버튼 클릭 처리 */
    UFUNCTION()
    void HandleRollButtonClicked();

protected:
    // 스킬명 텍스트
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SkillNameText;

    // 난이도 텍스트
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> DCText;

    // 수정치 텍스트
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ModifierText;

    // 결과 수치 텍스트
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ResultText;

    // 성공/실패 텍스트
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> OutcomeText;

    // 굴리기 버튼
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> RollButton;

    // 결과 표시 완료 여부.
    UPROPERTY(BlueprintReadWrite, Category = "Dice Roll")
    bool bResultShown = false;

private:
    // 현재 표시 중인 주사위 정보
    FPBDiceRollDisplayInfo CurrentInfo;
};
