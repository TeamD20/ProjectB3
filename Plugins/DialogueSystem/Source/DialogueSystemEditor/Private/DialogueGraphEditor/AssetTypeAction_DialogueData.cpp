// Copyright (c) 2026 TeamD20. All Rights Reserved.
#include "DialogueGraphEditor/AssetTypeAction_DialogueData.h"
#include "DialogueGraphEditor/DialogueDataEditorApp.h"

class FDialogueDataEditorApp;

FAssetTypeAction_DialogueData::FAssetTypeAction_DialogueData(EAssetTypeCategories::Type InAssetTypeCategories)
{
	AssetTypeCategories = InAssetTypeCategories;
}

FText FAssetTypeAction_DialogueData::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_DialogueData", "Dialogue Data");
}

FColor FAssetTypeAction_DialogueData::GetTypeColor() const
{
	return FColor(255, 128, 0); 
}

void FAssetTypeAction_DialogueData::OpenAssetEditor(const TArray<UObject*>& InObjects,
                                                    TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	// DialogueDataAsset을 열 때 커스텀 에디터를 열도록 함
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Obj : InObjects)
	{
		if (UDialogueData* DialogueData = Cast<UDialogueData>(Obj))
		{
			TSharedRef<FDialogueDataEditorApp> NewDialogueDataEditor = MakeShareable(new FDialogueDataEditorApp());
			NewDialogueDataEditor->InitEditor(Mode, EditWithinLevelEditor, DialogueData);
		}
	}
}
