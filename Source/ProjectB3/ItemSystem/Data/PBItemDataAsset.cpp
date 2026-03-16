// Copyright (c) 2026 TeamD20. All Rights Reserved.

#include "PBItemDataAsset.h"

FPrimaryAssetId UPBItemDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("PBItemData"), GetFName());
}
