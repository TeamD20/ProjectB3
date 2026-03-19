// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once
#include "AssetTypeActions_Base.h"
#include "DialogueSystemRuntime/Public/DialogueData.h"

class DIALOGUESYSTEMEDITOR_API FAssetTypeAction_DialogueData : public FAssetTypeActions_Base
{
public:
	FAssetTypeAction_DialogueData(EAssetTypeCategories::Type InAssetTypeCategories);

	/*~ FAssetTypeActions_Base Interface ~*/
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override { return UDialogueData::StaticClass(); }
	virtual uint32 GetCategories() override { return AssetTypeCategories; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

private:
	EAssetTypeCategories::Type AssetTypeCategories;
};
