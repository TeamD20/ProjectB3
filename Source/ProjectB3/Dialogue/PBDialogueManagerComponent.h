// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogueManagerComponent.h"
#include "ProjectB3/Dialogue/PBDialogueTypes.h"
#include "PBDialogueManagerComponent.generated.h"

class APBDialogueCameraActor;
class APBDiceRollActor;
class UPBDialogueViewModel;
class UPBDialogueWidget;

/**
 * 프로젝트 레벨 대화 관리 컴포넌트.
 * 라이프사이클(ViewModel 생성/정리, Widget Push/Pop)만 담당한다.
 * 모든 표시 갱신은 Feature의 OnStartDialogueNode에서 ViewModel을 직접 구동한다.
 */
UCLASS(ClassGroup = (Dialogue), meta = (BlueprintSpawnableComponent))
class PROJECTB3_API UPBDialogueManagerComponent : public UDialogueManagerComponent
{
    GENERATED_BODY()

public:
    /** ViewModel을 먼저 생성하고 Context에 등록한 뒤 Super::StartDialogue() 호출 */
    virtual void PreStartDialogue(UDialogueData* InDialogueData, const FDialogueSystemContext& InContext) override;
    
    /*~ UActorComponent Interface ~*/
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** 현재 대화 ViewModel 반환 */
    UFUNCTION(BlueprintPure, Category = "Dialogue")
    UPBDialogueViewModel* GetDialogueViewModel() const { return DialogueViewModel; }

    /** 주사위 굴리기 실행. ViewModel의 CachedDiceRollInfo에서 DC를 읽어 1d20+Mod 계산 */
    void RequestDiceRoll();

    /** 주사위 결과 확인 후 대화 진행. PendingDiceOptionId(0=성공, 1=실패)로 분기 */
    void ProgressDiceResult();

    /** 참여자 태그에 대한 대화 카메라 조회. 없으면 스폰 후 Map에 등록하여 반환 */
    APBDialogueCameraActor* GetOrCreateCamera(const FGameplayTag& InParticipantTag);

protected:
    /*~ UDialogueManagerComponent Interface ~*/
    /** Widget Push */
    virtual void OnDialogueStart(UDialogueNode* CurrentNode) override;

    /** 노드의 ParticipantTag로 화자 정보 구성 → ViewModel::SetSpeakerInfo() 호출 */
    virtual void OnDialogueChanged(UDialogueNode* CurrentNode) override;

    /** Widget Pop, ViewModel 정리, 카메라 파괴 */
    virtual void OnDialogueEnd(UDialogueNode* CurrentNode) override;

private:
    /** PlayerController에서 ViewModel을 생성하고 초기화 */
    void CreateAndInitViewModel(APlayerController* PC);

    /** Map에 등록된 모든 대화 카메라 파괴 및 Map 클리어 */
    void DestroyAllDialogueCameras();

    /** ParticipantTag 기반으로 화자 표시 정보를 구성하여 반환 */
    FPBDialogueParticipantDisplayInfo BuildSpeakerInfo(UDialogueNode* Node) const;

public:
    // Push할 대화 위젯 클래스
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    TSubclassOf<UPBDialogueWidget> DialogueWidgetClass;

    // BeginPlay 시 스폰할 주사위 액터 클래스. 미설정 시 3D 연출 없이 즉시 결과 처리
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Dice")
    TSubclassOf<APBDiceRollActor> DiceRollActorClass;

    // 주사위 액터 스폰 트랜스폼 (화면 밖 위치로 설정)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue|Dice")
    FTransform DiceActorSpawnTransform;

private:
    /** 주사위 굴림 애니메이션 완료 콜백 → ShowDiceResult 호출 */
    UFUNCTION()
    void OnDiceAnimationFinished(int32 ResultNumber);

    // 현재 대화 ViewModel
    UPROPERTY(Transient)
    TObjectPtr<UPBDialogueViewModel> DialogueViewModel;

    // BeginPlay에서 스폰되어 게임 내내 유지되는 주사위 액터
    UPROPERTY(Transient)
    TObjectPtr<APBDiceRollActor> DiceRollActor;

    // 애니메이션 완료 후 ShowDiceResult에 전달할 결과 (연출 중 임시 보관)
    FPBDiceRollDisplayInfo PendingDiceResult;

    // 주사위 결과 분기 인덱스 (0=성공, 1=실패). ProgressDiceResult에서 사용
    int32 PendingDiceOptionId = 0;

    // 현재 열린 대화 위젯 (약참조)
    TWeakObjectPtr<UPBDialogueWidget> CachedDialogueWidget;

    // 참여자 태그 : 대화 카메라 매핑
    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<APBDialogueCameraActor>> ParticipantCameraMap;
};
