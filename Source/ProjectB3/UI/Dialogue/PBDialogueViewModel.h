// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "PBDialogueViewModel.generated.h"

class UPBDialogueManagerComponent;

// 화자 정보 변경 이벤트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueSpeakerChanged, const FPBDialogueParticipantDisplayInfo&);
// 대사 텍스트 변경 이벤트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueTextChanged, const FText&);
// 선택지 변경 이벤트 (선택지 목록, 이전 노드 대사 프롬프트 텍스트)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDialogueChoicesChanged, const TArray<FPBDialogueChoiceInfo>&, const FText&);
// 주사위 굴리기 정보 변경 이벤트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueDiceRollChanged, const FPBDiceRollDisplayInfo&);
// 주사위 결과 변경 이벤트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDialogueDiceResultChanged, const FPBDiceRollDisplayInfo&);

/**
 * 대화 UI ViewModel.
 * Feature의 OnStartDialogueNode에서 ShowText/ShowChoices/ShowDiceRoll을 호출하여 직접 갱신된다.
 * Manager는 화자 정보(SetSpeakerInfo)만 설정한다.
 */
UCLASS()
class PROJECTB3_API UPBDialogueViewModel : public UPBViewModelBase
{
    GENERATED_BODY()

public:
    /*~ UPBViewModelBase Interface ~*/
    /** Manager를 연결하고 대화 상태를 초기화 */
    virtual void InitializeForPlayer(ULocalPlayer* InLocalPlayer) override;

    /** 내부 상태 및 Manager 참조 정리 */
    virtual void Deinitialize() override;

    /*~ 표시 메서드 (Feature가 호출) ~*/

    /** 화자 정보를 설정하고 OnSpeakerChanged를 브로드캐스트 */
    void SetSpeakerInfo(const FPBDialogueParticipantDisplayInfo& InInfo);

    /** 대사 텍스트를 설정하고 OnTextChanged를 브로드캐스트 */
    void ShowText(const FText& InText);

    /** 선택지를 설정하고 OnChoicesChanged를 브로드캐스트 */
    void ShowChoices(const TArray<FPBDialogueChoiceInfo>& InChoices);

    /** 주사위 굴리기 정보를 설정하고 OnDiceRollChanged를 브로드캐스트 */
    void ShowDiceRoll(const FPBDiceRollDisplayInfo& InInfo);

    /** 주사위 결과를 설정하고 OnDiceResultChanged를 브로드캐스트 */
    void ShowDiceResult(const FPBDiceRollDisplayInfo& InResult);

    /*~ 액션 메서드 (Widget이 호출) ~*/

    /** 텍스트 진행 요청 → Manager::ProgressDialogue(0) */
    void RequestContinue();

    /** 선택지 선택 → Manager::ProgressDialogue(Index) */
    void SelectChoice(int32 Index);

    /** 주사위 굴리기 요청 → Manager::RequestDiceRoll() */
    void RequestDiceRoll();

    /** 주사위 결과 확인 후 대화 진행 → Manager::ProgressDiceResult() */
    void RequestDiceProgress();

    /*~ Getter ~*/

    /** 현재 화자 정보 반환 */
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    const FPBDialogueParticipantDisplayInfo& GetSpeakerInfo() const { return CachedSpeakerInfo; }

    /** 현재 대사 텍스트 반환 */
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    FText GetDialogueText() const { return CachedDialogueText; }

    /** 현재 선택지 배열 반환 */
    const TArray<FPBDialogueChoiceInfo>& GetChoices() const { return CachedChoices; }

    /** 현재 주사위 정보 반환 */
    const FPBDiceRollDisplayInfo& GetDiceRollInfo() const { return CachedDiceRollInfo; }

    /** 선택지 표시 시 상단에 노출할 이전 노드 대사 텍스트 반환 */
    const FText& GetChoicePromptText() const { return CachedChoicePromptText; }


    /** Manager 참조 설정 (Manager가 생성 직후 호출) */
    void SetDialogueManager(UPBDialogueManagerComponent* InManager);

    /** 연결된 DialogueManager 반환 */
    UPBDialogueManagerComponent* GetDialogueManager() const { return DialogueManager.Get(); }

public:
    // 화자 정보 변경 이벤트
    FOnDialogueSpeakerChanged OnSpeakerChanged;
    // 대사 텍스트 변경 이벤트
    FOnDialogueTextChanged OnTextChanged;
    // 선택지 변경 이벤트
    FOnDialogueChoicesChanged OnChoicesChanged;
    // 주사위 굴리기 정보 변경 이벤트
    FOnDialogueDiceRollChanged OnDiceRollChanged;
    // 주사위 결과 변경 이벤트
    FOnDialogueDiceResultChanged OnDiceResultChanged;

private:
    // 현재 화자 정보
    FPBDialogueParticipantDisplayInfo CachedSpeakerInfo;
    // 현재 대사 텍스트
    FText CachedDialogueText;
    // 현재 선택지 배열
    TArray<FPBDialogueChoiceInfo> CachedChoices;
    // 현재 주사위 굴리기 정보
    FPBDiceRollDisplayInfo CachedDiceRollInfo;
    // 선택지 표시 시 상단에 노출할 이전 노드 대사 텍스트
    FText CachedChoicePromptText;
    // 연결된 Manager (약참조)
    TWeakObjectPtr<UPBDialogueManagerComponent> DialogueManager;
};
