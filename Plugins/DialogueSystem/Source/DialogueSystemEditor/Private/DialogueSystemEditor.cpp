// Copyright (c) 2026 TeamD20. All Rights Reserved.

#define LOCTEXT_NAMESPACE "FDialogueSystemEditorModule"
#include "DialogueSystemEditor.h"

#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DialogueGraphEditor/AssetTypeAction_DialogueData.h"
#include "DialogueGraphEditor/DialogueDataEditorConfig.h"
#include "DialogueGraphFactories/DialogueGraphNodeFactory.h"
#include "DialogueGraphFactories/DialogueGraphPinFactory.h"
#include "UObject/SavePackage.h"

class FAssetTypeAction_DialogueData;
class IAssetTools;
class SGraphPin;

void FDialogueSystemEditorModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FDialogueSystemEditorModule::OnPostEngineInit);
	
	IAssetTools& AssetToolsModule = IAssetTools::Get();
	EAssetTypeCategories::Type AssetType = AssetToolsModule.RegisterAdvancedAssetCategory(FName(TEXT("DialogueData")), LOCTEXT("DialogueData", "Dialogue Data"));
	DialogueDataAction = MakeShareable(new FAssetTypeAction_DialogueData(AssetType));
	AssetToolsModule.RegisterAssetTypeActions(DialogueDataAction.ToSharedRef());
	
	PinFactory = MakeShareable(new FDialogueGraphPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);

	GraphNodeFactory = MakeShareable(new FDialogueGraphNodeFactory());
	FEdGraphUtilities::RegisterVisualNodeFactory(GraphNodeFactory);
}

void FDialogueSystemEditorModule::ShutdownModule()
{
	FEdGraphUtilities::UnregisterVisualPinFactory(PinFactory);
	FEdGraphUtilities::UnregisterVisualNodeFactory(GraphNodeFactory);
}

UDialogueDataEditorConfig* FDialogueSystemEditorModule::GetDialogueDataEditorConfig()
{
	const FString AssetPath = GetConfigAssetPackagePath() + GetConfigAssetName() + TEXT(".") + GetConfigAssetName();
	UDialogueDataEditorConfig* ConfigAsset = LoadObject<UDialogueDataEditorConfig>(nullptr, *AssetPath);
	if (!ConfigAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load DialogueDataEditorConfig asset at path: %s"), *AssetPath);
	}
	return ConfigAsset;
}


void FDialogueSystemEditorModule::OnPostEngineInit()
{
	CreateEditorConfigIfNeeded(); // 설정 에셋 생성 (없을 경우)
}
void FDialogueSystemEditorModule::CreateEditorConfigIfNeeded()
{
    // 폴더 경로와 에셋 이름을 분리해서 정의
    FString ConfigAssetFolder = GetConfigAssetPackagePath();
    FString ConfigAssetName = GetConfigAssetName(); 
    
    // 전체 패키지 이름 생성 (에셋 이름을 포함)
    FString PackageName = ConfigAssetFolder + ConfigAssetName;
    const FString FullAssetPath = PackageName + TEXT(".") + ConfigAssetName;

    // 기존에 에셋이 존재하는지 확인 (중복 생성 방지)
    UDialogueDataEditorConfig* ConfigAsset = LoadObject<UDialogueDataEditorConfig>(nullptr, *FullAssetPath);
    if (ConfigAsset)
    {
        return;
    }

    // 폴더 존재 여부 확인 후 없으면 생성
    const FString EditorFolderRelativePath = TEXT("Editor/DialogueDataEditor");
    FString FolderPath = FPaths::ProjectContentDir() / EditorFolderRelativePath;
    if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*FolderPath))
    {
        if (FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FolderPath))
        {
            UE_LOG(LogTemp, Log, TEXT("Folder '%s' created successfully."), *FolderPath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create folder '%s'."), *FolderPath);
        }
    }

    // 패키지 생성 (전체 패키지 이름 사용)
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to create package for CustomEditorConfig."));
        return;
    }

    // 패키지 내에서 DataAsset 생성 (에디터에서 인식될 수 있도록 플래그 설정)
    ConfigAsset = NewObject<UDialogueDataEditorConfig>(Package, UDialogueDataEditorConfig::StaticClass(), FName(*ConfigAssetName), RF_Public | RF_Standalone);
    if (!ConfigAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create CustomEditorConfig asset."));
        return;
    }

    // 패키지에 변경 사항 표시 및 에셋 레지스트리에 등록
    Package->MarkPackageDirty();
    FAssetRegistryModule::AssetCreated(ConfigAsset);

    // 에셋 저장
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
    bool bSaved = UPackage::SavePackage(Package, ConfigAsset, *PackageFileName, SaveArgs);
    if (bSaved)
    {
        UE_LOG(LogTemp, Log, TEXT("CustomEditorConfig asset created and saved successfully."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save the CustomEditorConfig asset."));
    }
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDialogueSystemEditorModule, DialogueSystemEditor)