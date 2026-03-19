/*
 * Class: DialogueSystemEditor
 * Author: Yuchan Bae
 * Created: 2025년 2월 22일 토요일
 * Description: (Write DESCRIPTION)
 */
#pragma once
#include "Modules/ModuleManager.h"

class UDialogueDataEditorConfig;

class FDialogueSystemEditorModule : public IModuleInterface
{
public:

	 /*~ IModuleInterface Interface ~*/
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	 /*~ FDialogueSystemEditorModule Interface ~*/
	static UDialogueDataEditorConfig* GetDialogueDataEditorConfig();

private:
	void OnPostEngineInit();
	void CreateEditorConfigIfNeeded();
public:
	static FString GetConfigAssetPackagePath()
	{
		return TEXT("/Game/Editor/DialogueDataEditor/");
	}
	static FString GetConfigAssetName()
	{
		return TEXT("DialogueDataEditorConfig");
	}
	
private:
	TSharedPtr<class FAssetTypeAction_DialogueData> DialogueDataAction = nullptr;
	TSharedPtr<struct FDialogueGraphPinFactory> PinFactory = nullptr;
	TSharedPtr<struct FDialogueGraphNodeFactory> GraphNodeFactory = nullptr;
};
