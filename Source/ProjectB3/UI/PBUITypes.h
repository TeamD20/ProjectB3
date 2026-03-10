#pragma once

namespace PBUIDelegate
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTextChangedSignature, FText);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnImageChangedSignature, TSoftObjectPtr<UTexture2D>);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnFloatValueChangedSignature, float);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBoolValueChangedSignature, bool);
}

namespace PBSkillBarTabIndex
{
	static constexpr int32 Action = 0;
	static constexpr int32 BonusAction = 1;
	static constexpr int32 Spell = 2;
}