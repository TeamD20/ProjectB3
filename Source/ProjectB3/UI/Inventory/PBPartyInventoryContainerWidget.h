// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/PBWidgetBase.h"
#include "PBPartyInventoryContainerWidget.generated.h"

class APBGameplayPlayerState;
class UHorizontalBox;
class UPBInventoryPanelWidget;

// 파티원 인벤토리 패널들을 배치하는 최상위 컨테이너 위젯
UCLASS()
class PROJECTB3_API UPBPartyInventoryContainerWidget : public UPBWidgetBase
{
	GENERATED_BODY()

protected:
	/*~ UUserWidget Interface ~*/
	// 파티원 목록 변경 이벤트를 바인딩하고 패널을 구성
	virtual void NativeConstruct() override;

	// 파티원 목록 변경 이벤트를 해제
	virtual void NativeDestruct() override;

private:
	// 현재 파티원 목록 기반으로 패널을 재구성
	void RebuildPanels();

	// 바인딩된 PlayerState를 갱신
	void CachePlayerState();

	// 파티원 목록 변경 이벤트를 처리
	void HandlePartyMembersChanged();

protected:
	// 파티원 패널을 담는 컨테이너
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> PartyPanelBox;

	// 개별 파티원 패널 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Inventory", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPBInventoryPanelWidget> PanelWidgetClass;

private:
	// 현재 바인딩된 PlayerState
	UPROPERTY(Transient)
	TObjectPtr<APBGameplayPlayerState> CachedPlayerState;

	// 생성된 패널 위젯 캐시
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPBInventoryPanelWidget>> ActivePanels;

	// 파티원 목록 변경 델리게이트 핸들
	FDelegateHandle PartyMembersChangedHandle;
};
