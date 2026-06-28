// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 최승현 (프로젝트 모듈 시작 코드 작성)

#include "ProjectB3.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "AI/Debug/PBGameplayDebuggerCategory_AI.h"
#endif

class FProjectB3Module : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		IGameplayDebugger& GDModule = IGameplayDebugger::Get();
		GDModule.RegisterCategory(
			"UtilityAI",
			IGameplayDebugger::FOnGetCategory::CreateStatic(
				&FPBGameplayDebuggerCategory_AI::MakeInstance),
			EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
		GDModule.NotifyCategoriesChanged();
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		if (IGameplayDebugger::IsAvailable())
		{
			IGameplayDebugger& GDModule = IGameplayDebugger::Get();
			GDModule.UnregisterCategory("UtilityAI");
		}
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FProjectB3Module, ProjectB3, "ProjectB3");

DEFINE_LOG_CATEGORY(LogProjectB3)
