// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "DialogueSystemRuntime/Public/DialogueSystemRuntime.h"

#define LOCTEXT_NAMESPACE "FDialogueSystemRuntimeModule"

void FDialogueSystemRuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FDialogueSystemRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDialogueSystemRuntimeModule, DialogueSystemRuntime)