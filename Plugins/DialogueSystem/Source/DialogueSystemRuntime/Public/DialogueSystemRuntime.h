// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "Modules/ModuleManager.h"

class FDialogueSystemRuntimeModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
