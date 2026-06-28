// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (UI 화면과 위젯 식별 태그 작성)

#include "PBUITags.h"

namespace PBGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_Test, "UI.Test", "This is Test UI GameplayTag");
}

namespace PBUITags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_ViewModel_TurnOrder, "UI.ViewModel.TurnOrder", "Turn Order ViewModel Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_ViewModel_CharacterStat, "UI.ViewModel.CharacterStat", "Character Stat ViewModel Tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(UI_ViewModel_SkillBar, "UI.ViewModel.SkillBar", "SkillBar ViewModel Tag");
}