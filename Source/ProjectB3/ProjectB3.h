// Copyright (c) 2026 TeamD20. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogProjectB3, Log, All);

// 프로젝트 공용 트레이스 채널 정의
namespace PBTraceChannel
{
	constexpr ECollisionChannel Ground = ECollisionChannel::ECC_GameTraceChannel1;
	constexpr ECollisionChannel Combat = ECollisionChannel::ECC_GameTraceChannel2;
}

namespace PBStencilValues
{
	constexpr int32 DEFAULT = 0;
	constexpr int32 HIGHLIGHT = 1;
	constexpr int32 INTERACTION = 2;
	constexpr int32 TARGET_FRIENDLY = 3;
	constexpr int32 TARGET_HOSTILE = 4;
}