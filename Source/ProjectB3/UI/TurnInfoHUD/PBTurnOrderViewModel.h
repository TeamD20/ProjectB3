// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "ProjectB3/UI/PBUITypes.h"
#include "PBTurnPortraitViewModel.h"
#include "PBTurnOrderViewModel.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, meta = (DisplayName = "Turn Order ViewModel"))
class PROJECTB3_API UPBTurnOrderViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	// 외부(GameMode 등)에서 전체 턴 목록을 한 번에 주입 시 호출
	UFUNCTION(BlueprintCallable, Category = "Turn|Order")
	void SetTurnOrder(const TArray<FPBTurnOrderEntry>& InData);

	// 현재 턴 종료 및 다음 턴으로 넘기기
	UFUNCTION(BlueprintCallable, Category = "Turn|Order")
	void AdvanceTurn();

	// 특정 인덱스 플레이어의 사망 상태 업데이트
	UFUNCTION(BlueprintCallable, Category = "Turn|Order")
	void UpdatePortraitState(int32 TargetIndex, bool bIsDead);

	// Getters
	UFUNCTION(BlueprintPure, Category = "Turn|Order")
	const TArray<UPBTurnPortraitViewModel*>& GetPortraitViewModels() const { return PortraitViewModels; }

	UFUNCTION(BlueprintPure, Category = "Turn|Order")
	int32 GetCurrentTurnIndex() const { return CurrentTurnIndex; }

public:
	// 리스트 전체 변경 신호 (위젯들의 카드 재구성용)
	PBUIDelegate::FOnTurnOrderListChangedSignature OnTurnOrderListChanged;

	// 애니메이션(인디케이터) 알림용 신호 (누구의 턴인지 전달)
	PBUIDelegate::FOnTurnAdvancedSignature OnTurnAdvanced;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn|Order")
	TArray<UPBTurnPortraitViewModel*> PortraitViewModels;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Turn|Order")
	int32 CurrentTurnIndex = -1;
};
