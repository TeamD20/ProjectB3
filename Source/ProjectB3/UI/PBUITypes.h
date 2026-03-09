#pragma once
#include "TurnInfoHUD/PBTurnOrderViewModel.h"
#include "TurnInfoHUD/PBTurnPortraitViewModel.h"

namespace PBUIDelegate
{
	/* ~파티 / 또는 공용 델리게이트 ~ */ 
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTextChangedSignature, FText);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnImageChangedSignature, TSoftObjectPtr<UTexture2D>);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFloatValueChangedSignature, float);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoolValueChangedSignature, bool);
	
	/* ~턴오더 UI 델리게이트~ */ 
	DECLARE_MULTICAST_DELEGATE(FOnTurnOrderListChagedSignature);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnAdvancedSignature, UPBTurnOrderViewModel* NewTurnOrder);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnStateChangedSignature, UPBTurnPortraitViewModel*);
}
