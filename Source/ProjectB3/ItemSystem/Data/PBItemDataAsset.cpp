// Copyright (c) 2026 TeamD20. All Rights Reserved.
// Author: 배유찬 (아이템 표시값과 사용 설정 작성)

#include "PBItemDataAsset.h"

FPrimaryAssetId UPBItemDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("PBItemData"), GetFName());
}
