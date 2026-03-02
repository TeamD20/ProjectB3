// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBExampleTurnOrderViewModel.generated.h"

class UTexture2D;

// 턴 순서 내 각 항목
USTRUCT(BlueprintType)
struct FPBTurnOrderEntry
{
	GENERATED_BODY()

	// 표시 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	// 초상화 아이콘 (연출용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> Portrait;

	// Initiative 굴림 결과
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Initiative = 0;

	// 아군 여부 (색상 분기에 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAlly = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTurnOrderChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnAdvanced, int32, NewIndex);

/**
 * 턴 기반 전투의 턴 순서 표시를 위한 Global ViewModel 예시
 */
UCLASS(BlueprintType, meta = (DisplayName = "Turn Order ViewModel"))
class PROJECTB3_API UPBExampleTurnOrderViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	UPBExampleTurnOrderViewModel();

	// Setter (전투 시작/캐릭터 추가·제거 시 호출)
	UFUNCTION(BlueprintCallable, Category = "Turn Order")
	void SetTurnOrder(const TArray<FPBTurnOrderEntry>& InOrder);

	// 턴 넘김
	UFUNCTION(BlueprintCallable, Category = "Turn Order")
	void AdvanceTurn();

	// 라운드 갱신
	UFUNCTION(BlueprintCallable, Category = "Turn Order")
	void SetRoundNumber(int32 InRound);

	// Getter (Presentation Logic)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn Order")
	FPBTurnOrderEntry GetCurrentEntry() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn Order")
	FText GetRoundText() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Turn Order")
	int32 GetRemainingTurnsInRound() const;

	// 턴 순서 배열 반환
	const TArray<FPBTurnOrderEntry>& GetTurnOrder() const { return TurnOrder; }

	// 현재 턴 인덱스 반환
	int32 GetCurrentTurnIndex() const { return CurrentTurnIndex; }

	// 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Turn Order|Events")
	FOnTurnOrderChanged OnTurnOrderChanged;

	UPROPERTY(BlueprintAssignable, Category = "Turn Order|Events")
	FOnTurnAdvanced OnTurnAdvanced;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Turn Order")
	TArray<FPBTurnOrderEntry> TurnOrder;

	UPROPERTY(BlueprintReadOnly, Category = "Turn Order")
	int32 CurrentTurnIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Turn Order")
	int32 RoundNumber = 1;
};
