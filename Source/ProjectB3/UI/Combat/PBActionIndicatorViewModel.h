// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectB3/UI/ViewModel/PBViewModelBase.h"
#include "PBActionIndicatorTypes.h"
#include "PBActionIndicatorViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnActionIndicatorChangedSignature, const FPBActionIndicatorData&);

/**
 * 행동 인디케이터 상태를 관리하는 ViewModel
 */
UCLASS()
class PROJECTB3_API UPBActionIndicatorViewModel : public UPBViewModelBase
{
	GENERATED_BODY()

public:
	// 행동 설정
	void SetAction(const FPBActionIndicatorData& NewAction);

	// 행동 해제
	void ClearAction();

	// 현재 행동 데이터 조회
	const FPBActionIndicatorData& GetCurrentAction() const { return CurrentAction; }

public:
	// 인디케이터 변경 델리게이트
	FOnActionIndicatorChangedSignature OnActionChanged;

private:
	// 현재 동작 중인 행동
	FPBActionIndicatorData CurrentAction;
};
