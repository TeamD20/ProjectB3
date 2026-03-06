#pragma once
#include "NativeGameplayTags.h"

namespace PBUITags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Test);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_ViewModel_TurnOrder);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_ViewModel_CharacterStat);
}

namespace PBUIDelegate
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTextChangedSignature, FText);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnImageChangedSignature, TSoftObjectPtr<UTexture2D>);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFloatValueChangedSignature, float);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoolValueChangedSignature, bool);
}